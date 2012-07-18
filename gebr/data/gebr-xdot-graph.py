import pygtk
import sys, gtk, glib
import xdot
import gettext

class MyDotWindow(xdot.DotWindow):
    def __init__(self):
        xdot.DotWindow.__init__(self)
        self.widget.connect('clicked', self.on_url_clicked)
        self.widget.connect('activate', self.on_url_activate)

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

    def on_revert_clicked(self, widget, url):
        revert_str = "revert:"+url
        sys.stderr.write(str(revert_str))
        self.menu.destroy()

    def on_delete_clicked(self, widget, url):
        delete_str = "delete:"+url
        sys.stderr.write(str(delete_str))
        self.menu.destroy()
        
    def on_snapshot_clicked(self, widget, url):
        sys.stderr.write(str("snapshot"))
        self.menu.destroy()
    
    def on_url_clicked(self, widget, url, event):
        self.menu = gtk.Menu()
        self.menu.popup(None, None, None, event.button, event.time, url)
        if url == "head":
            snapshot = gtk.MenuItem(_("Take a Snapshot"))
            snapshot.connect("activate", self.on_snapshot_clicked, url)
            snapshot.show()
            self.menu.append(snapshot)
        else:
            revert = gtk.MenuItem(_("Revert"))
            revert.connect("activate", self.on_revert_clicked, url)
            revert.show()
            self.menu.append(revert)
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

    def create_dot_graph(self, fd, condition):
        if condition == glib.IO_IN:
           dotfile = fd.readline()
           self.set_dotcode(dotfile)
           return True

def main():
    window = MyDotWindow()

    glib.io_add_watch(sys.stdin, # FILE DESCRIPTOR
                      glib.IO_IN,  # CONDITION
                      window.create_dot_graph) # CALLBACK

    window.connect('destroy', gtk.main_quit)

    gtk.main()

if __name__ == '__main__':
    main()
