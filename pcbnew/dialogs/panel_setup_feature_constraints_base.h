///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/valtext.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_FEATURE_CONSTRAINTS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_FEATURE_CONSTRAINTS_BASE : public wxPanel
{
	private:

	protected:
		wxCheckBox* m_OptAllowBlindBuriedVias;
		wxCheckBox* m_OptAllowMicroVias;
		wxStaticLine* m_staticline2;
		wxStaticText* m_stCircleToPolyOpt;
		wxStaticText* m_maxErrorTitle;
		wxTextCtrl* m_maxErrorCtrl;
		wxStaticText* m_maxErrorUnits;
		wxBoxSizer* m_bSizerPolygonFillOption;
		wxStaticLine* m_staticline1;
		wxStaticText* m_stZoneFilledPolysOpt;
		wxCheckBox* m_cbOutlinePolygonBestQ;
		wxCheckBox* m_cbOutlinePolygonFastest;
		wxStaticText* m_TrackMinWidthTitle;
		wxTextCtrl* m_TrackMinWidthCtrl;
		wxStaticText* m_TrackMinWidthUnits;
		wxStaticText* m_ViaMinTitle;
		wxTextCtrl* m_SetViasMinSizeCtrl;
		wxStaticText* m_ViaMinUnits;
		wxStaticText* m_ViaMinDrillTitle;
		wxTextCtrl* m_SetViasMinDrillCtrl;
		wxStaticText* m_ViaMinDrillUnits;
		wxStaticText* m_uviaMinSizeLabel;
		wxTextCtrl* m_uviaMinSizeCtrl;
		wxStaticText* m_uviaMinSizeUnits;
		wxStaticText* m_uviaMinDrillLabel;
		wxTextCtrl* m_uviaMinDrillCtrl;
		wxStaticText* m_uviaMinDrillUnits;
		wxStaticText* m_HoleToHoleTitle;
		wxTextCtrl* m_SetHoleToHoleCtrl;
		wxStaticText* m_HoleToHoleUnits;
		wxStaticText* m_EdgeClearanceLabel;
		wxTextCtrl* m_EdgeClearanceCtrl;
		wxStaticText* m_EdgeClearanceUnits;

		// Virtual event handlers, overide them in your derived class
		virtual void onChangeOutlineOpt( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_FEATURE_CONSTRAINTS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_SETUP_FEATURE_CONSTRAINTS_BASE();

};

