#include "FL/Fl.H"
#include "FL/Fl_Window.H"


int main(int argc, char ** argv) {
	Fl_Window *window = new Fl_Window(320,130);
	window->end();
	window->show(argc,argv);
	return Fl::run();
}
