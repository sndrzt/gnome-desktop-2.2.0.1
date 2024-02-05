/*
 * gnome-about
 * (c) 1999-2000 the Free Software Foundation
 *
 * Informative little about thing that lets us brag to our friends as
 * our name scrolls by, and lets users click to load the GNOME
 * homepages. (no easter eggs here)
 */

#include <config.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <libgnomeui/gnome-window-icon.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include "contributors.h"
#include "logo.xpm"

gboolean cb_quit      (GtkWidget *widget, gpointer data);
gboolean cb_exposed   (GtkWidget *widget, GdkEventExpose *event);
gboolean cb_configure (GtkWidget *widget, GdkEventConfigure *event);
gboolean cb_keypress  (GtkWidget *widget, GdkEventKey *event);
gboolean cb_clicked   (GtkWidget *widget, GdkEvent *event);

static GtkWidget   *area   = NULL;
static GdkPixmap   *pixmap = NULL;
static PangoLayout *layout = NULL;
static int          font_descent;
static int          font_ascent;
static int          fast_fwd = 0;
static char        *gnome_version_string = NULL;
static char        *gnome_vendor_string = NULL;
static char        *gnome_build_date_string = NULL;
static char        *gnome_intro_message = NULL;

int howmuch = 0;

GnomeCanvasItem *image;
GtkWidget       *canvas;
GdkPixbuf       *im;
	
static gint sparkle_timer = -1;
static gint scroll_timer = -1;
static gint new_sparkle_timer = -1;

/* Sparkles */
#define NUM_FRAMES 8

typedef struct _Sparkle Sparkle;
struct _Sparkle {
	GnomeCanvasItem *hline;
	GnomeCanvasItem *vline;
	GnomeCanvasItem *hsmall;
	GnomeCanvasItem *vsmall;
	
	GnomeCanvasPoints *hpoints [NUM_FRAMES];
	GnomeCanvasPoints *vpoints [NUM_FRAMES];
	GnomeCanvasPoints *hspoints [NUM_FRAMES];
	GnomeCanvasPoints *vspoints [NUM_FRAMES];

	gint count;
	gboolean up;
};


static void
sparkle_destroy (Sparkle *sparkle)
{
	int i;
	g_return_if_fail (sparkle != NULL);
	
	gtk_object_destroy (GTK_OBJECT (sparkle->hline));
	gtk_object_destroy (GTK_OBJECT (sparkle->vline));
	gtk_object_destroy (GTK_OBJECT (sparkle->hsmall));
	gtk_object_destroy (GTK_OBJECT (sparkle->vsmall));

	for (i=0; i > NUM_FRAMES ; i++) {
		gnome_canvas_points_free (sparkle->hpoints [i]);
		gnome_canvas_points_free (sparkle->vpoints [i]);
		gnome_canvas_points_free (sparkle->hspoints [i]);
		gnome_canvas_points_free (sparkle->vspoints [i]);
	}
	g_free (sparkle);
}

static gboolean
sparkle_timeout (Sparkle *sparkle)
{
	g_return_val_if_fail (sparkle != 0, FALSE);

	if (sparkle->count == -1) {
		sparkle_destroy (sparkle);
		return FALSE;
	}

	gnome_canvas_item_set (sparkle->hline, "points",
			       sparkle->hpoints [sparkle->count], NULL);
	gnome_canvas_item_set (sparkle->vline, "points",
			       sparkle->vpoints [sparkle->count], NULL);
	gnome_canvas_item_set (sparkle->hsmall, "points",
			       sparkle->hspoints [sparkle->count], NULL);
	gnome_canvas_item_set (sparkle->vsmall, "points",
			       sparkle->vspoints [sparkle->count], NULL);

	if (sparkle->count == NUM_FRAMES - 1)
		sparkle->up = FALSE;

	if (sparkle->up)
		sparkle->count++;
	else
		sparkle->count--;

	return TRUE;
}

static void
fill_points (GnomeCanvasPoints *points, double x, double y, double delta,
	     gboolean horizontal, gboolean square)
{
	if (horizontal) {
		if (square) {
			points->coords[0] = x - delta;
			points->coords[1] = y;
			points->coords[2] = x + delta;
			points->coords[3] = y;
		}
		else {
			points->coords[0] = x - delta;
			points->coords[1] = y - delta;
			points->coords[2] = x + delta;
			points->coords[3] = y + delta;
		}
	}
	else {
		if (square) {
			points->coords[0] = x;
			points->coords[1] = y - delta;
			points->coords[2] = x;
			points->coords[3] = y + delta;
		}
		else {
			points->coords[0] = x + delta;
			points->coords[1] = y - delta;
			points->coords[2] = x - delta;
			points->coords[3] = y + delta;
		}
	}  
}

#define DELTA 0.4

static void
sparkle_new (GnomeCanvas *canvas, double x, double y)
{
	int i;
	double delta;

	Sparkle *sparkle = g_new (Sparkle, 1);
	GnomeCanvasPoints *points = gnome_canvas_points_new (2);

	fill_points (points, x, y, 0.1, TRUE, TRUE);
	sparkle->hsmall = gnome_canvas_item_new(GNOME_CANVAS_GROUP(canvas->root),
						gnome_canvas_line_get_type(),
						"points", points,
						"fill_color", "light gray",
						"width_units", 1.0,
						NULL);
	
	gnome_canvas_item_raise_to_top(sparkle->hsmall);
	
	fill_points(points, x, y, 0.1, FALSE, TRUE);
	sparkle->vsmall = gnome_canvas_item_new(GNOME_CANVAS_GROUP(canvas->root),
						gnome_canvas_line_get_type(),
						"points", points,
						"fill_color", "light gray",
						"width_units", 1.0,
						NULL);
	
	gnome_canvas_item_raise_to_top(sparkle->vsmall);
	
	fill_points(points, x, y, DELTA, TRUE, TRUE);
	sparkle->hline = gnome_canvas_item_new(GNOME_CANVAS_GROUP(canvas->root),
					       gnome_canvas_line_get_type(),
					       "points", points,
					       "fill_color", "white",
					       "width_units", 1.0,
					       NULL);
	
	fill_points(points, x, y, DELTA, FALSE, TRUE);
	sparkle->vline = gnome_canvas_item_new(GNOME_CANVAS_GROUP(canvas->root),
					       gnome_canvas_line_get_type(),
					       "points", points,
					       "fill_color", "white",
					       "width_units", 1.0,
					       NULL);
	
	gnome_canvas_points_free(points);
	
	i = 0;
	delta = 0.0;
	while ( i < NUM_FRAMES ) {
		sparkle->hpoints[i] = gnome_canvas_points_new(2);
		sparkle->vpoints[i] = gnome_canvas_points_new(2);
		sparkle->hspoints[i] = gnome_canvas_points_new(2);
		sparkle->vspoints[i] = gnome_canvas_points_new(2);
		
		
		fill_points(sparkle->hspoints[i], x, y, delta, TRUE, FALSE);    
		fill_points(sparkle->vspoints[i], x, y, delta, FALSE, FALSE);    
		
		delta += DELTA;
		fill_points(sparkle->hpoints[i], x, y, delta, TRUE, TRUE);
		fill_points(sparkle->vpoints[i], x, y, delta + delta*.70, FALSE, TRUE);
		++i;
	}
	
	sparkle->count = 0;
	sparkle->up = TRUE;
	
	sparkle_timer = gtk_timeout_add (75,(GtkFunction)sparkle_timeout, sparkle);
}

static gint 
new_sparkles_timeout (GnomeCanvas* canvas)
{
	static gint which_sparkle = 0;

	if (howmuch >= 5)
	        return TRUE;

	switch (which_sparkle) {
	case 0:
		sparkle_new(canvas,130.0,10.0);
		break;
	case 1: 
		sparkle_new(canvas,45.0,150.0);
		break;
	case 2:
		sparkle_new(canvas,45.0,70.0);
		break;
	case 3: 
		sparkle_new(canvas,111.0,155.0);
		break;
	case 4: 
		sparkle_new(canvas,110.0,65.0);
		break;
	case 5: 
		sparkle_new(canvas,115.0,110.0);
		break;
	default:
		which_sparkle = -1;
		break;
	};
	
	++which_sparkle;
	return TRUE;
}

static void
set_tooltip_foreach_link (GtkWidget   *href,
			  GtkTooltips *tooltips)
{
	const char *url;
	char       *description;

	g_return_if_fail (GTK_IS_WIDGET (href));
	g_return_if_fail (GTK_IS_TOOLTIPS (tooltips));
	
	url = gnome_href_get_url (GNOME_HREF (href));
	description = g_strconcat (
			_("Click here to visit the site : "),
			url, NULL);
	gtk_tooltips_set_tip (tooltips, href, description, NULL);
	g_free (description);
}

static void
make_contributors_logo_accessible (GtkTooltips *tooltips)
{
	AtkObject *aobj;
	
	gtk_tooltips_set_tip (tooltips, area,
			      _("List of GNOME Contributors"), NULL);
	gtk_tooltips_set_tip (tooltips, canvas,
			      _("GNOME Logo Image"), NULL);

	aobj = gtk_widget_get_accessible (area);

	/* Check if gail is loaded */
	if (GTK_IS_ACCESSIBLE (aobj) == FALSE)
		return;

	atk_object_set_name (aobj, _("Contributors' Names"));
	aobj = gtk_widget_get_accessible (canvas);
	atk_object_set_name (aobj, _("GNOME Logo"));
}

gboolean
cb_clicked (GtkWidget *widget, GdkEvent *event)
{
	if (event->type == GDK_BUTTON_PRESS && howmuch >= 5) {
		char *filename;
			
		filename = gnome_program_locate_file (
				NULL, GNOME_FILE_DOMAIN_DATADIR,
				"gnome-about/contributors.dat", TRUE, NULL);
		if (filename)
			gnome_sound_play (filename);

		g_free (filename);

	}

	return FALSE;
}

gboolean
cb_keypress (GtkWidget *widget, GdkEventKey *event)
{
	if (howmuch >= 5)
		return FALSE;

	switch (event->keyval) {
	case GDK_e:
	case GDK_E:
		if (howmuch == 4) {
			howmuch++;
			im = gdk_pixbuf_new_from_xpm_data (magick);
			gnome_canvas_item_set (image,
					       "pixbuf", im,
					       NULL);
		}
		else
			howmuch = 0;
		break;
	case GDK_g:
	case GDK_G:
		if (howmuch == 0)
			howmuch++;
		else
			howmuch = 0;
		break;
	case GDK_m:
	case GDK_M:
		if (howmuch == 3)
			howmuch ++;
		else
			howmuch = 0;
		break;
	case GDK_n:
	case GDK_N:
		if (howmuch == 1)
			howmuch++;
		else
			howmuch = 0;
		break;
	case GDK_o:
	case GDK_O:
		if (howmuch == 2)
			howmuch++;
		else
			howmuch = 0;
		break;
	default:
		howmuch = 0;
	}

	return FALSE;
}

gboolean
cb_quit (GtkWidget *widget, gpointer data)
{
	GtkTooltips *tooltips;

	tooltips = g_object_get_data (G_OBJECT (widget), "tooltips");
	if (tooltips) {
		g_object_unref (tooltips);
		g_object_set_data (G_OBJECT (widget), "tooltips", NULL);
	}

	if (sparkle_timer != -1)
		gtk_timeout_remove (sparkle_timer);
	if (scroll_timer != -1)
		gtk_timeout_remove (scroll_timer);
	if (new_sparkle_timer != -1)
		gtk_timeout_remove (new_sparkle_timer);

	g_object_unref (layout);

	gtk_main_quit ();

	return FALSE; /* causes "destroy" signal to be emitted */
}

static enum {
	DRAWING_INTRO,
	DRAWING_BOTH,
	DRAWING_NAMES,
	DRAWING_END
} state = DRAWING_INTRO;

static void
mouseclick_scroll (GtkWidget       *widget,
		   GdkEventButton  *event)
{
	if (event->button == 1)
		fast_fwd++;
}

static void
mousewheel_scroll (GtkWidget *widget,
		   GdkEventScroll *event)
{
	if (event->direction == GDK_SCROLL_UP)
		fast_fwd--;
	else if (event->direction == GDK_SCROLL_DOWN)
		fast_fwd++;
}

static gboolean
keypress_scroll (GtkWidget       *widget,
		GdkEventKey  *event)
{
	if (event->keyval == GDK_space)
		fast_fwd++;
		
	return FALSE;
}

static void
init_version_strings (void)
{
	xmlDocPtr   about;
	xmlNodePtr  node;
	xmlNodePtr  bits;
	char       *platform = NULL;
	char       *minor = NULL;
	char       *micro = NULL;
	char       *vendor = NULL;

	about = xmlParseFile (gnome_program_locate_file (
			      NULL, GNOME_FILE_DOMAIN_DATADIR,
			      "gnome-about/gnome-version.xml", TRUE, NULL) );
	if (!about) {
		gnome_version_string = g_strdup ("");
		return;
	}

	node = about->children;
	
	if (g_ascii_strcasecmp (node->name, "gnome-version")) {
		gnome_version_string = g_strdup ("");
		xmlFreeDoc (about);
		return;
	}

	bits = node->children;

	while (bits) {
		char *name  = (char*) bits->name;
		char *value = (char*) xmlNodeGetContent (bits);

		if (!g_ascii_strcasecmp (name, "platform") && value && value [0])
			platform = g_strdup (value);
		if (!g_ascii_strcasecmp (name, "minor") && value && value [0])
			minor = g_strdup (value);
		if (!g_ascii_strcasecmp (name, "micro") && value && value [0])
			micro = g_strdup (value);
		if (!g_ascii_strcasecmp (name, "vendor") && value && value [0])
			gnome_vendor_string = g_strdup (value);
		if (!g_ascii_strcasecmp (name, "date") && value && value [0])
			gnome_build_date_string = g_strdup (value);
		
		bits = bits->next;
	}

	xmlFreeDoc (about);

	if (!platform)
		gnome_version_string = g_strdup ("");

	if (!gnome_version_string && !minor)
		gnome_version_string = g_strconcat (platform, vendor, NULL);

	if (!gnome_version_string && !micro)
		gnome_version_string = g_strconcat (platform, ".", minor, vendor, NULL);

	if (!gnome_version_string)
		gnome_version_string = g_strconcat (platform, ".", minor, ".", micro, vendor, NULL);

	g_free (platform);
	g_free (minor);
	g_free (micro);
	g_free (vendor);
}

static char *
get_intro_message (void)
{
	if (!gnome_intro_message)
		gnome_intro_message =
			g_strdup_printf ("%s %s %s",
				  _("GNOME"),
				  gnome_version_string ? gnome_version_string : "",
				  ("Was Brought To You By"));

	return gnome_intro_message;
}
	
static void
draw_intro (void)
{
	static int  initial_pause = 50;
	static int  x = G_MININT;
	static int  y = G_MININT;
	static int  layout_width = 0;
	char       *markup;

	markup = g_strdup_printf ("<big><b>%s</b></big>", get_intro_message ());
	pango_layout_set_markup (layout, markup, -1);
	g_free (markup);

	if (x == G_MININT) {
		PangoRectangle extents;

		pango_layout_get_pixel_extents (layout, NULL, &extents);
		
		x = (area->allocation.width - extents.width) / 2;
		y = (area->allocation.height - extents.height) / 2;
		layout_width = extents.width;
	}

	gdk_draw_layout (pixmap, area->style->white_gc, x, y, layout);

	if (fast_fwd  != 0) {
		state = (fast_fwd > 0) ? DRAWING_NAMES : DRAWING_END;
		fast_fwd = MAX (0, fast_fwd);
		x = - layout_width;
		initial_pause = 50;
		return;
	}

	if (x == (area->allocation.width - layout_width) / 2 && initial_pause-- >= 0)
		return;

	x++;
	if (x > area->allocation.width) {
		state = DRAWING_NAMES;
		x = - layout_width;
		initial_pause = 50;

	} else if ((x + (layout_width / 2)) > area->allocation.width)
		state = DRAWING_BOTH;

}

static void
draw_names ()
{
	PangoRectangle extents;
	static int     index = -1;
	static int     going_y = G_MININT;
	static int     middle_y = G_MININT;
	static int     coming_y = G_MININT;
	int            middle_height = 0;

	if (middle_y == G_MININT)
		middle_y = area->allocation.height / 2 - font_ascent;

	if (fast_fwd != 0) {
		(fast_fwd > 0) ? index++ : index--;
		fast_fwd = 0;
		coming_y = G_MININT;
		going_y = middle_y - font_ascent - font_descent;
		
		if (index < 0) {
			index = G_N_ELEMENTS (contributors) - 2;
			going_y  = G_MININT;
			state = DRAWING_END;
			return;
		}
		
		if (index >= G_N_ELEMENTS (contributors) - 2) {
			index = -1;
			index = -1;
			going_y  = G_MININT;
			middle_y = G_MININT;
			coming_y = G_MININT;
			state = DRAWING_END;
			return;
		}
	}

	if (index - 1 >= 0 && going_y != G_MININT) {
		pango_layout_set_markup (layout, _(contributors [index - 1]), -1);
		pango_layout_get_pixel_extents (layout, NULL, &extents);

		gdk_draw_layout (pixmap, area->style->white_gc,
				 (area->allocation.width - extents.width) / 2,
				 going_y, layout);

		going_y--;
	}

	if (index >= 0 && index < G_N_ELEMENTS (contributors)) {
		char *markup;

		markup = g_strdup_printf ("<big><b>%s</b></big>", _(contributors [index]));
		pango_layout_set_markup (layout, markup, -1);
		g_free (markup);

		pango_layout_get_pixel_extents (layout, NULL, &extents);

		gdk_draw_layout (pixmap, area->style->white_gc,
				 (area->allocation.width - extents.width) / 2,
				 middle_y, layout);

		middle_height = extents.height;
	}

	if (index + 1 < G_N_ELEMENTS (contributors)) {
		if (coming_y == G_MININT)
			coming_y = area->allocation.height + font_ascent;

		pango_layout_set_markup (layout, _(contributors [index + 1]), -1);
		pango_layout_get_pixel_extents (layout, NULL, &extents);

		gdk_draw_layout (pixmap, area->style->white_gc,
				 (area->allocation.width - extents.width) / 2,
				 coming_y, layout);

		coming_y--;
	}

	if (coming_y != G_MININT && coming_y <= middle_y + middle_height) {
		index++;
		coming_y = G_MININT;
		going_y = middle_y - font_ascent - font_descent;
	}

	if (index + 1 >= G_N_ELEMENTS (contributors)) {
		index = -1;
		going_y  = G_MININT;
		middle_y = G_MININT;
		coming_y = G_MININT;
		state = DRAWING_END;
	}
}

static void
draw_end ()
{
	PangoRectangle  extents;
	static int      pause = 100;
	char           *markup;

	markup = g_strdup_printf ("<big><b>%s</b></big>", _("And Many More ..."));
	pango_layout_set_markup (layout, markup, -1);
	g_free (markup);

	pango_layout_get_pixel_extents (layout, NULL, &extents);

	gdk_draw_layout (pixmap, area->style->white_gc,
			(area->allocation.width - extents.width) / 2,
			(area->allocation.height - extents.height) / 2,
			layout);

	if (fast_fwd != 0) {
		state = DRAWING_NAMES;
		pause = 100;
		return;
	}

	if (pause-- == 0) {
		state = DRAWING_INTRO;
		pause = 100;
	}
}

#define NUM_STARS 150

static void
draw_stars ()
{
	static gint starx[NUM_STARS];
	static gint stary[NUM_STARS];
	static gint starz[NUM_STARS];
	static gboolean inited=FALSE;

	gint i=0;
	gint x=0, y=0;

	if (!inited) {
		gint depth=0;
		for (i = 0; i < NUM_STARS; i++) {
			starx[i] = (random()%50) - 25;
			stary[i] = (random()%50) - 25;
			starz[i] = depth;
			depth+=150/NUM_STARS;
		}
		inited=TRUE;
	}

	for (i = 0; i < NUM_STARS; i++) {
		x = (((double)starx[i]/(double)starz[i])*625) +
			area->allocation.width/2;
		y = (((double)stary[i]/(double)starz[i])*625) +
			area->allocation.height/2;
	
		gdk_draw_point (pixmap, area->style->white_gc, x, y);
		
		starz[i]-=1;
		if (starz[i] < 0 ) starz[i] = 150;
	}
}

static gboolean
scroll (gpointer dummy)
{
	gdk_draw_rectangle (pixmap, area->style->black_gc,
			    TRUE, 0, 0,
			    area->allocation.width,
			    area->allocation.height);

	draw_stars ();

	switch (state) {
	case DRAWING_INTRO:
		draw_intro ();
		break;
	case DRAWING_NAMES:
		draw_names ();
		break;
	case DRAWING_BOTH:
		draw_intro ();
		draw_names ();
		break;
	case DRAWING_END:
		draw_end ();
		break;
	}

	gtk_widget_queue_draw_area (area, 0, 0,
				    area->allocation.width,
				    area->allocation.height);

	return TRUE;
}

gboolean
cb_exposed (GtkWidget *widget, GdkEventExpose *event)
{
	gdk_draw_drawable (widget->window,
			   widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
			   pixmap,
			   event->area.x, event->area.y,
			   event->area.x, event->area.y,
			   event->area.width, event->area.height);
	return TRUE;
}

gboolean
cb_configure (GtkWidget *widget, GdkEventConfigure *event)
{
	g_return_val_if_fail (pixmap == NULL, FALSE);

	pixmap = gdk_pixmap_new (widget->window,
				 widget->allocation.width,
				 widget->allocation.height,
				 -1);

	return TRUE;
}

static gint
get_max_width (void)
{
	PangoRectangle extents;
	int            max_width = 0;
	int            i;
	
	for (i = 0; i < G_N_ELEMENTS (contributors); i++) {
		pango_layout_set_text (layout, _(contributors [i]), -1);
		pango_layout_get_pixel_extents (layout, NULL, &extents);

		max_width = MAX (max_width, extents.width);
	}

	pango_layout_set_text (layout, get_intro_message (), -1);
	pango_layout_get_pixel_extents (layout, NULL, &extents);
	max_width = MAX (max_width, extents.width);

	return max_width + 4;
}

gint
main (gint argc, gchar *argv[])
{
	GtkWidget        *window;
	GtkWidget        *hbox;
	GtkWidget        *alignment;
	GtkWidget        *vbox;
	GtkWidget        *label;
	GdkPixmap        *logo_pixmap;
	GdkBitmap        *logo_mask;
	GtkWidget        *frame;
	GtkWidget        *gtkpixmap;
	GtkTooltips      *tooltips;
	GList            *hrefs;
	PangoContext     *context;
	PangoFontMetrics *metrics;
	int               max_width;
	char             *title;
	char             *vendor_string = NULL;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gnome_program_init ("gnome-about","1.0", LIBGNOMEUI_MODULE, argc, argv, NULL);
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-logo-icon-transparent.png");
	gtk_widget_set_default_colormap (gdk_rgb_get_colormap ());

	init_version_strings ();
	g_assert (gnome_version_string);

	if (gnome_vendor_string && gnome_build_date_string)
		vendor_string = g_strdup_printf (" (%s - %s)",
						 gnome_vendor_string,
						 gnome_build_date_string);

	else if (gnome_vendor_string && !gnome_build_date_string)
		vendor_string = g_strdup_printf (" (%s)", gnome_vendor_string);

	title = g_strdup_printf (_("About GNOME%s%s%s"),
				 gnome_version_string[0] ? " " : "" ,
				 gnome_version_string,
				 vendor_string ? vendor_string : "");
	window = gtk_dialog_new_with_buttons (title, NULL, 0,
					      GTK_STOCK_OK, GTK_RESPONSE_OK,	
				   	      NULL);
	g_free (title);
	g_free (vendor_string);

	gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_OK);

	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
	g_object_set (window, "allow_shrink", FALSE, "allow_grow", FALSE, NULL);

	gtk_widget_realize (window);

	logo_pixmap = gdk_pixmap_create_from_xpm_d (window->window, &logo_mask,
						    NULL,
						    (char **)logo_xpm);
	gtkpixmap = gtk_image_new_from_pixmap (logo_pixmap, logo_mask);

	im = gdk_pixbuf_new_from_xpm_data (logo_xpm);
	canvas = gnome_canvas_new ();
	gtk_widget_set_size_request (canvas,
			             gdk_pixbuf_get_width (im),
			             gdk_pixbuf_get_height (im));
	gnome_canvas_set_scroll_region (GNOME_CANVAS (canvas), 0, 0,
					gdk_pixbuf_get_width (im),
					gdk_pixbuf_get_height (im));

	image = gnome_canvas_item_new (GNOME_CANVAS_GROUP (GNOME_CANVAS (canvas)->root),
				       gnome_canvas_pixbuf_get_type (),
				       "pixbuf", im,
				       "x", 0.0,
				       "y", 0.0,
				       "width", (double) gdk_pixbuf_get_width (im),
				       "height", (double) gdk_pixbuf_get_height (im),
				       NULL);

	g_object_unref (im);
	
	g_signal_connect (window, "delete_event",
			  G_CALLBACK (cb_quit), im);
	g_signal_connect (window, "destroy",
			  G_CALLBACK (cb_quit), im);
	g_signal_connect (window, "key_press_event",
			  G_CALLBACK (cb_keypress), NULL);

	g_signal_connect (image, "event",
			  G_CALLBACK (cb_clicked), NULL);

	new_sparkle_timer = gtk_timeout_add (1300,
					     (GtkFunction) new_sparkles_timeout,
					     canvas);

	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (window)->vbox), GNOME_PAD);

	hbox = gtk_hbox_new (FALSE, 10);
	vbox = gtk_vbox_new (FALSE, 10);
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (frame), canvas);
	gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);

	area = gtk_drawing_area_new ();
	GTK_WIDGET_SET_FLAGS(area, GTK_CAN_FOCUS);
	layout = gtk_widget_create_pango_layout (area, NULL);
	context = gtk_widget_get_pango_context (area);
	metrics = pango_context_get_metrics (context,
					     area->style->font_desc,
					     pango_context_get_language (context));
	font_descent = pango_font_metrics_get_descent (metrics) / PANGO_SCALE;
	font_ascent  = pango_font_metrics_get_ascent  (metrics) / PANGO_SCALE;
	pango_font_metrics_unref (metrics);

	max_width = get_max_width();
	if (max_width < 320) 
		GTK_WIDGET (area)->requisition.width = 320;
	else
		GTK_WIDGET (area)->requisition.width = max_width;
	GTK_WIDGET (area)->requisition.height = 160;
	gtk_widget_queue_resize (GTK_WIDGET (area));
	gtk_widget_queue_draw (area);

	gtk_widget_add_events (area, GDK_BUTTON_PRESS_MASK);
	gtk_widget_add_events (area, GDK_KEY_PRESS_MASK);
	g_signal_connect (area, "button_press_event",
			  G_CALLBACK (mouseclick_scroll), NULL);
	g_signal_connect (area, "key_press_event",
			  G_CALLBACK (keypress_scroll), NULL);
	g_signal_connect (area, "scroll_event",
			  G_CALLBACK (mousewheel_scroll), NULL);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (frame), area);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), frame);

	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (window)->vbox), hbox);

	g_signal_connect (area, "expose_event",
			  G_CALLBACK (cb_exposed), NULL);
	g_signal_connect (area, "configure_event",
			  G_CALLBACK (cb_configure), NULL);

	/* horizontal box for URLs */
	hbox = gtk_hbox_new (TRUE, 10);

	gtk_box_pack_start (GTK_BOX (hbox),
			    gnome_href_new ("http://www.gnomedesktop.org/",
					    _("GNOME News Site")),
			    TRUE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox),
			    gnome_href_new (_("http://www.gnome.org/"),
					    _("GNOME Main Site")),
			    TRUE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox),
			    gnome_href_new ("http://developer.gnome.org/",
					    _("GNOME Developers' Site")),
			    TRUE, FALSE, 0);
	
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (window)->vbox), hbox);


	g_signal_connect (window, "response",
			  G_CALLBACK (cb_quit), NULL);

	scroll_timer = gtk_timeout_add (20, scroll, NULL);

	tooltips = gtk_tooltips_new ();
	g_object_ref (tooltips);
	gtk_object_sink (GTK_OBJECT (tooltips));
	g_object_set_data (G_OBJECT (window), "tooltips", tooltips);

	hrefs = gtk_container_get_children (GTK_CONTAINER (hbox));
	g_list_foreach (hrefs, (GFunc) set_tooltip_foreach_link, tooltips);
	g_list_free (hrefs);


	label = gnome_href_new ("http://www.gnu.org/",
					    _("GNOME is a part of the GNU Project"));
	gtk_button_set_relief (GTK_BUTTON(label), GTK_RELIEF_NONE);
	alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
	gtk_container_add (GTK_CONTAINER (alignment), label);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (window)->vbox), alignment);
    
	set_tooltip_foreach_link (label, tooltips);
    
	make_contributors_logo_accessible (tooltips);

	gtk_widget_show_all (window);
	gtk_main ();

	g_free (gnome_version_string);
	g_free (gnome_vendor_string);
	g_free (gnome_build_date_string);
	g_free (gnome_intro_message);

	return 0;
}
