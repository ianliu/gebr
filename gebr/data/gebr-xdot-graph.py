import pygtk
import sys, gtk, glib
import xdot
import gettext

class MyDotWindow(xdot.DotWindow):
    def __init__(self):
        xdot.DotWindow.__init__(self)
        self.widget.connect('clicked', self.on_url_clicked)
        self.widget.connect('activate', self.on_url_activate)
        self.widget.connect('select', self.on_url_select)
        self.widget.connect('unselect-all', self.on_url_unselect_all)
        self.widget.connect("focus-in-event", self.on_focus_in_event)
        self.widget.connect("focus-out-event", self.on_focus_out_event)
        self.widget.connect('key-press-event', self.on_key_press_event)
        
        self.flows = {}
        self.current_flow = None

        Wid = 0L
        if len(sys.argv) == 3:
            Wid = long(sys.argv[1])
            locale_dir = str(sys.argv[2])

        self.plug = gtk.Plug(Wid)

        self.container = gtk.VBox(False, 0)
        self.widget.reparent(self.container)

        self.plug.add(self.container)
        self.plug.show_all()

        self.window.destroy()
        
        gettext.install("gebr", locale_dir)
        
    def on_focus_in_event(self, widget, event):
        sys.stderr.write("focus-in:\n")
        sys.stderr.flush()
        return True
    
    def on_focus_out_event(self, widget, event):
        sys.stderr.write("focus-out:\n")
        sys.stderr.flush()
        return True
    
    def delete_selected_snapshots(self):
        if self.flows.has_key(self.current_flow) and self.flows[self.current_flow]:
            delete_str = "delete:"
            for snapshot in self.flows[self.current_flow]:
                delete_str = delete_str + snapshot + ","
            delete_str = delete_str[:-1]
            sys.stderr.write(str(delete_str))
        
    def on_key_press_event(self, widget, event):
         if event.keyval == gtk.keysyms.Delete:
              if self.flows.has_key(self.current_flow) and "head" not in self.flows[self.current_flow]:
                  self.delete_selected_snapshots()
         return False
    
    def on_revert_clicked(self, widget, url):
        revert_str = "revert:"+url
        sys.stderr.write(str(revert_str))
        self.menu.destroy()

    def on_delete_clicked(self, widget, url):
        delete_str = "delete:"
        if self.flows.has_key(self.current_flow) and self.flows[self.current_flow]:
            for snapshot in self.flows[self.current_flow]:
                delete_str = delete_str + snapshot + ","
            delete_str = delete_str[:-1]
        else:
            delete_str = delete_str + url
            
        sys.stderr.write(str(delete_str))
        self.menu.destroy()
        
    def on_snapshot_clicked(self, widget, url):
        sys.stderr.write(str("snapshot"))
        self.menu.destroy()
        
    def on_url_unselect_all(self, widget, url, event):
       if not self.flows or not self.flows.has_key(self.current_flow):
           return
       for snapshot in self.flows[self.current_flow]:
           node = self.widget.graph.get_node_by_url(snapshot)
           self.widget.on_area_unselect_node(node)
       self.flows[self.current_flow] = []
       sys.stderr.write("unselect:")
    
    def on_url_select(self, widget, url, event):
        current = self.current_flow
        id = url.split(":")
        action = id[0]
        
        widget.grab_focus()
        
        if not self.flows.has_key(current):
             selection = [id[1]]
             if action == "select":
                 self.flows[current] = selection
        else:
            if id[1] not in self.flows[current]:
                self.flows[current].append(id[1])
            elif action == "unselect":
                self.flows[current].remove(id[1])
             
        if self.flows[current]:
            sys.stderr.write("select:")
        else:
            sys.stderr.write("unselect:")            
    
    def on_url_clicked(self, widget, url, event):
        self.menu = gtk.Menu()
        self.menu.popup(None, None, None, event.button, event.time, url)
        if url == "head":
            snapshot = gtk.MenuItem(_("Take a Snapshot"))
            snapshot.connect("activate", self.on_snapshot_clicked, url)
            snapshot.show()
            self.menu.append(snapshot)
        else:
            has_snap = False
            if self.flows.has_key(self.current_flow):
                if url in self.flows[self.current_flow]:
                    has_snap = True
                if not has_snap:
                    self.on_url_unselect_all(widget, url, event)
                
            revert = gtk.MenuItem(_("Revert"))
            revert.connect("activate", self.on_revert_clicked, url)
            revert.show()
            self.menu.append(revert)
            if self.flows.has_key(self.current_flow) and "head" in self.flows[self.current_flow]:
                self.menu.show_all()
                return True
            else: 
                delete = gtk.MenuItem(_("Delete"))
                delete.connect("activate", self.on_delete_clicked, url)
                delete.show()
                self.menu.append(delete)
        
        self.menu.show_all()
        return True

    def on_url_activate(self, widget, url, event):
        if url == "head":
            sys.stderr.write(str("snapshot"))
        else:
            revert_str = "revert:"+url
            sys.stderr.write(str(revert_str))
        return True

    def on_input_response(self, fd, condition):
        if condition == glib.IO_IN:
           file = fd.readline()
           info = file.split("\b")
           
           if info[0] == "run":
               list_snaps = "run:"+info[1].strip()+":"
               self.flows[self.current_flow].sort()
               for snap in self.flows[self.current_flow]:
                   list_snaps = list_snaps + snap + ","
               sys.stderr.write(str(list_snaps[:-1])) 
           
           elif info[0] == "draw":
               # Get ID Flow (Filename)
               id_flow = info[1]
               # Option to keep selection or delete
               keep_selection = info[2]
               # Get dot code to generate graph
               dotfile = info[3]
               
               self.set_dotcode(dotfile)
               
               if keep_selection == "no":
                   self.flows[self.current_flow] = []
    
               # Set on dictionary a dotfile with id of flow
               if self.flows.has_key(id_flow):
                   for snapshot in self.flows[id_flow]:
                       node = self.widget.graph.get_node_by_url(snapshot)
                       self.widget.on_area_select_node(node, True)
                       
               self.current_flow = id_flow

           return True

def main():
    window = MyDotWindow()

    glib.io_add_watch(sys.stdin, # FILE DESCRIPTOR
                      glib.IO_IN,  # CONDITION
                      window.on_input_response) # CALLBACK

    window.connect('destroy', gtk.main_quit)

    gtk.main()

if __name__ == '__main__':
    main()
