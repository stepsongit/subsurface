#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "display.h"

int selected_dive = 0;

#define ROUND_UP(x,y) ((((x)+(y)-1)/(y))*(y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

static int round_seconds_up(int seconds)
{
	return MAX(30, ROUND_UP(seconds, 60*10));
}

static int round_feet_up(int feet)
{
	return MAX(45, ROUND_UP(feet+5, 15));
}

/* Scale to 0,0 -> maxx,maxy */
#define SCALE(x,y) (x)*maxx/scalex+topx,(y)*maxy/scaley+topy

static void plot(cairo_t *cr, int w, int h, struct dive *dive, int samples, struct sample *sample)
{
	int i;
	double topx, topy, maxx, maxy;
	double scalex, scaley;
	int maxtime, maxdepth;

	topx = w / 20.0;
	topy = h / 20.0;
	maxx = (w - 2*topx);
	maxy = (h - 2*topy);

	cairo_set_line_width(cr, 2);

	/* Get plot scaling limits */
	maxtime = round_seconds_up(dive->duration.seconds);
	maxdepth = round_feet_up(to_feet(dive->maxdepth));

	/* Depth markers: every 15 ft */
	scalex = 1.0;
	scaley = maxdepth;
	cairo_set_source_rgba(cr, 1, 1, 1, 0.5);
	for (i = 15; i < maxdepth; i += 15) {
		cairo_move_to(cr, SCALE(0, i));
		cairo_line_to(cr, SCALE(1, i));
	}

	/* Time markers: every 5 min */
	scalex = maxtime;
	scaley = 1.0;
	for (i = 5*60; i < maxtime; i += 5*60) {
		cairo_move_to(cr, SCALE(i, 0));
		cairo_line_to(cr, SCALE(i, 1));
	}
	cairo_stroke(cr);

	scaley = maxdepth;

	/* Depth profile */
	cairo_set_source_rgba(cr, 1, 0.2, 0.2, 0.80);
	cairo_move_to(cr, SCALE(sample->time.seconds, to_feet(sample->depth)));
	for (i = 1; i < dive->samples; i++) {
		sample++;
		cairo_line_to(cr, SCALE(sample->time.seconds, to_feet(sample->depth)));
	}
	cairo_stroke(cr);

	/* Bounding box last */
	scalex = scaley = 1.0;
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_move_to(cr, SCALE(0,0));
	cairo_line_to(cr, SCALE(0,1));
	cairo_line_to(cr, SCALE(1,1));
	cairo_line_to(cr, SCALE(1,0));
	cairo_close_path(cr);
	cairo_stroke(cr);

}

static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	struct dive *dive = dive_table.dives[selected_dive];
	cairo_t *cr;
	int w,h;

	w = widget->allocation.width;
	h = widget->allocation.height;

	cr = gdk_cairo_create(widget->window);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_paint(cr);

	if (dive->samples)
		plot(cr, w, h, dive, dive->samples, dive->sample);

	cairo_destroy(cr);

	return FALSE;
}

GtkWidget *dive_profile_frame(void)
{
	GtkWidget *frame;
	GtkWidget *da;

	frame = gtk_frame_new("Dive profile");
	gtk_widget_show(frame);
	da = gtk_drawing_area_new();
	gtk_widget_set_size_request(da, 450, 350);
	g_signal_connect(da, "expose_event", G_CALLBACK(expose_event), NULL);
	gtk_container_add(GTK_CONTAINER(frame), da);

	return frame;
}
