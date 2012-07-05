import pygtk
import sys, gtk, glib
import xdot

class MyDotWindow(xdot.DotWindow):

    def __init__(self):
        xdot.DotWindow.__init__(self)
        self.widget.connect('clicked', self.on_url_clicked)

        Wid = 0L
        if len(sys.argv) == 2:
            Wid = long(sys.argv[1])

        print "Window ID is "+str(Wid)

        self.plug = gtk.Plug(Wid)

        self.container = gtk.VBox(False, 0)
        self.widget.reparent(self.container)

        self.plug.add(self.container)
        self.plug.show_all()

        self.window.destroy()

    def on_url_clicked(self, widget, url, event):
        sys.stderr.write(str(url))
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
