import pygtk
import sys, gtk, glib
import xdot
import gettext
import gtk.keysyms

class MyDotWidget(xdot.DotWidget):
    def __init__(self):
        xdot.DotWidget.__init__(self)
        
        self.connect('clicked', self.on_url_clicked)
        self.connect('activate', self.on_url_activate)
        self.connect('select', self.on_url_select)
        self.connect('unselect-all', self.on_url_unselect_all)
        self.connect('key-press-event', self.on_url_key_press)
        
        self.flows = {}
        self.current_flow = None

        Wid = 0L
        if len(sys.argv) == 3:
            Wid = long(sys.argv[1])
            locale_dir = str(sys.argv[2])

        self.plug = gtk.Plug(Wid)

        self.plug.add(self)
        self.plug.show_all()

        gettext.install("gebr", locale_dir)
        
    def on_url_key_press(self, widget, event):
        if event.keyval == gtk.keysyms.Delete:
            self.delete_selected_snapshots()
        
    def delete_selected_snapshots(self):
        if self.flows.has_key(self.current_flow) and self.flows[self.current_flow]:
            delete_str = "delete:"
            for snapshot in self.flows[self.current_flow]:
                if snapshot == "head":
                    return
                delete_str = delete_str + snapshot + ","
            delete_str = delete_str[:-1]
            sys.stderr.write(str(delete_str))
        
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
           node = self.graph.get_node_by_url(snapshot)
           self.on_area_unselect_node(node)
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
            
            
    # Methods to execute flow
    def on_execute_single(self, widget, url):
        snap = "run:single:default:" + url
        sys.stderr.write(str(snap))
        
    def create_list_and_execution(self, type, mod):
        list_snaps = "run:" + type + ":" + mod + ":"
        self.flows[self.current_flow].sort()
        for snap in self.flows[self.current_flow]:
            list_snaps = list_snaps + snap + ","
        sys.stderr.write(str(list_snaps[:-1]))  
    
    def on_execute_seq(self, widget, url):
        self.create_list_and_execution("single", "default")
        
    def on_execute_parallel(self, widget, url):
        self.create_list_and_execution("parallel", "default")
    
    
    def on_url_clicked(self, widget, url, event):
        self.menu = gtk.Menu()
        self.menu.popup(None, None, None, event.button, event.time, url)
        
        multiple_selection = False
        has_snap = False
        if self.flows.has_key(self.current_flow):
            if url in self.flows[self.current_flow]:
                has_snap = True
            if not has_snap:
                self.on_url_unselect_all(widget, url, event)
            if len(self.flows[self.current_flow]) > 1:
                multiple_selection = True
                
        if url == "head" and not multiple_selection:
            snapshot = gtk.MenuItem(_("Take a Snapshot \tCtrl+S"))
            snapshot.connect("activate", self.on_snapshot_clicked, url)
            snapshot.show()
            self.menu.append(snapshot)
            
             # Add separator
            separator =  gtk.SeparatorMenuItem()
            separator.show()
            self.menu.append(separator)
        else:
            if self.flows.has_key(self.current_flow):
                is_head = False
                if "head" in self.flows[self.current_flow]:
                    is_head = True
            
            if not is_head:
                if not multiple_selection:
                    revert = gtk.MenuItem(_("Revert"))
                    revert.connect("activate", self.on_revert_clicked, url)
                    revert.show()
                    self.menu.append(revert)
                    
                delete = gtk.MenuItem(_("Delete"))
                delete.connect("activate", self.on_delete_clicked, url)
                delete.show()
                self.menu.append(delete)
                
                # Add separator
                separator =  gtk.SeparatorMenuItem()
                separator.show()
                self.menu.append(separator)
            
        # Insert execution menu
        if multiple_selection:
            # Sequentially
            execute_single = gtk.MenuItem(_("Run sequentially \t\tCtrl+R"))
            execute_single.connect("activate", self.on_execute_seq, url)
            execute_single.show()
            self.menu.append(execute_single)
            # Parallel
            execute_paral = gtk.MenuItem(_("Run parallelly \t\tCtrl+Shift+R"))
            execute_paral.connect("activate", self.on_execute_parallel, url)
            execute_paral.show()
            self.menu.append(execute_paral)
        else:
            execute_single = gtk.MenuItem(_("Run \t\t\tCtrl+R"))
            execute_single.connect("activate", self.on_execute_single, url)
            execute_single.show()
            self.menu.append(execute_single)
            
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
               self.create_list_and_execution(info[1].strip(), info[2].strip())
           
           elif info[0] == "delete":
               id = info[1]
               self.delete_selected_snapshots()
               
           elif info[0] == "unselect-all":
               self.on_url_unselect_all(None, None, None)
           
           elif info[0] == "draw":
               # Get ID Flow (Filename)
               id_flow = info[1]
               # Option to keep selection or delete
               keep_selection = info[2]
               # Get dot code to generate graph
               dotfile = info[3]
               
               self.set_dotcode(dotfile)
               self.zoom_to_fit()
               
               if keep_selection == "no":
                   self.on_url_unselect_all(None, None, None)
    
               # Set on dictionary a dotfile with id of flow
               if self.flows.has_key(id_flow):
                   for snapshot in self.flows[id_flow]:
                       node = self.graph.get_node_by_url(snapshot)
                       self.on_area_select_node(node, True)
               else:
                   self.flows[id_flow] = []
                
               self.current_flow = id_flow

           return True

def main():
    widget = MyDotWidget()

    glib.io_add_watch(sys.stdin, # FILE DESCRIPTOR
                      glib.IO_IN,  # CONDITION
                      widget.on_input_response) # CALLBACK

    gtk.main()

if __name__ == '__main__':
    main()
