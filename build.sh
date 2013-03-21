#makes life easier
gcc graph.c -I/usr/local/include -I/opt/X11/include -L/usr/local/lib -I/opt/X11/lib -w -framework OpenGL -framework Cocoa -framework IOKit -lglfw -o graph && echo "built as ./graph"