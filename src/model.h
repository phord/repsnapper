/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#ifndef MODEL_H
#define MODEL_H

#include <math.h>

#include <giomm/file.h>

//#include "linked_ptr.h"

//#include "slicer.h"
#include "objtree.h"
#include "types.h"
#include "slicer/gcode.h"
/*#include "slicer/layer.h"*/
#include "settings.h"
#include "progress.h"
#include "slicer/poly.h"

#ifdef _MSC_VER // Visual C++ compiler
#  pragma warning( disable : 4244 4267)
#endif


class Model
{
  
	sigc::signal< void > m_signal_tree_changed;
	ViewProgress *m_progress;

public:
	Gtk::Statusbar *statusbar;
	// Something in the rfo changed
	sigc::signal< void > signal_tree_changed() { return m_signal_tree_changed; }

	Model();
	~Model();

	void SimpleAdvancedToggle();
	void SaveConfig(Glib::RefPtr<Gio::File> file);
	void LoadConfig() { LoadConfig(Gio::File::create_for_path("repsnapper.conf")); }
	void LoadConfig(Glib::RefPtr<Gio::File> file);

	// STL Functions
	void ReadStl(Glib::RefPtr<Gio::File> file, filetype_t ftype=UNKNOWN_TYPE);
	void SaveStl(Glib::RefPtr<Gio::File> file);
	int AddShape(TreeObject *parent, Shape shape, string filename,
		     bool autoplace = true);
	int SplitShape(TreeObject *parent, Shape shape, string filename);
	int DivideShape(TreeObject *parent, Shape shape, string filename);

	sigc::signal< void, Gtk::TreePath & > m_signal_stl_added;

	void Read(Glib::RefPtr<Gio::File> file);
	void SetViewProgress (ViewProgress *progress);

	void DeleteObjTree(Gtk::TreeModel::iterator &iter);

	void OptimizeRotation(Shape *shape, TreeObject *object);
	void ScaleObject(Shape *shape, TreeObject *object, double scale);
	void ScaleObjectX(Shape *shape, TreeObject *object, double scale);
	void ScaleObjectY(Shape *shape, TreeObject *object, double scale);
	void ScaleObjectZ(Shape *shape, TreeObject *object, double scale);
	void RotateObject(Shape *shape, TreeObject *object, Vector4d rotate);
	void TwistObject(Shape *shape, TreeObject *object, double angle);
	void PlaceOnPlatform(Shape *shape, TreeObject *object);
	bool updateStatusBar(GdkEventCrossing *event, Glib::ustring = "");
	void InvertNormals(Shape *shape, TreeObject *object);
	void Mirror(Shape *shape, TreeObject *object);

	vector<Layer*> layers;
	
	Layer * m_previewLayer;
	Layer * m_previewGCodeLayer;
	
	// Slicing
	void Slice(double printoffsetZ);
	void CalcInfill();
	void MakeShells();
	void MakeUncoveredPolygons(bool make_decor, bool make_bridges=true);
	vector<Poly> GetUncoveredPolygons(const Layer *subjlayer, 
					  const Layer *cliplayer);
	vector<ExPoly> GetUncoveredExPolygons(const Layer * subjlayer,
					      const Layer * cliplayer);
	void MakeFullSkins();
	void MultiplyUncoveredPolygons();
	void MakeSupportPolygons(Layer * subjlayer, const Layer * cliplayer, 
				 double widen=0);
	void MakeSupportPolygons(double widen=0);
	void MakeSkirt();

	// GCode Functions
	void init();
	void ReadGCode(Glib::RefPtr<Gio::File> file);
	void ConvertToGCode();
	void MakeRaft(GCodeState &state, double &z);
	void WriteGCode(Glib::RefPtr<Gio::File> file);
	void ClearGCode();
	void ClearLayers();
	void ClearPreview();
	Glib::RefPtr<Gtk::TextBuffer> GetGCodeBuffer();
	void GlDrawGCode(int layer=-1); // should be in the view
	void GlDrawGCode(double Z); 
	void setCurrentPrintingLine(long fromline, long toline){
	  currentprintingline = toline;
	  currentbufferedlines = (int)(toline-fromline);
	}
	unsigned long currentprintingline;
	int currentbufferedlines;

	Matrix4f &SelectedNodeMatrix(guint objectNr = 1);
	void SelectedNodeMatrices(vector<Matrix4d *> &result );
	void newObject();


	Settings settings;

	// Model derived: Bounding box info
	Vector3d Center;
	Vector3d Min;
	Vector3d Max;

	void CalcBoundingBoxAndCenter();
	Vector3d GetViewCenter();
        bool FindEmptyLocation(Vector3d &result, Shape *stl);

	sigc::signal< void > m_model_changed;
	void ModelChanged();

	// Truly the model
	ObjectsTree objtree;
	Glib::RefPtr<Gtk::TextBuffer> errlog, echolog;

	int draw(Gtk::TreeModel::iterator &selected);
	int drawLayers(double height, Vector3d offset, bool calconly = false);
	void setMeasuresPoint(const Vector2d point);
	Vector2d measuresPoint;

	Layer * calcSingleLayer(double z, uint LayerNr, double thickness, 
				bool calcinfill, bool for_gcode=false) const ;

	sigc::signal< void, Gtk::MessageType, const char *, const char * > signal_alert;
	void alert (const char *message);
	void error (const char *message, const char *secondary);

	void ClearLogs();

	GCode gcode;

 private:
	bool is_calculating;
	//GCodeIter *m_iter;
};

#endif // MODEL_H
