/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/* Fields are texts attached to a component, having a special meaning
 * Fields 0 and 1 are very important: reference and value
 * Field 2 is used as default footprint name.
 * Field 3 is reserved (not currently used
 * Fields 4 and more are user fields.
 * They can be renamed and can appear in reports
 */

#include <fctsys.h>
#include <base_struct.h>
#include <gr_basic.h>
#include <gr_text.h>
#include <macros.h>
#include <sch_edit_frame.h>
#include <plotter.h>
#include <bitmaps.h>
#include <kiway.h>
#include <general.h>
#include <class_library.h>
#include <sch_component.h>
#include <sch_field.h>
#include <settings/color_settings.h>
#include <kicad_string.h>
#include <trace_helpers.h>


SCH_FIELD::SCH_FIELD( const wxPoint& aPos, int aFieldId, SCH_ITEM* aParent, const wxString& aName ) :
    SCH_ITEM( aParent, SCH_FIELD_T ),
    EDA_TEXT()
{
    SetTextPos( aPos );
    m_id = aFieldId;
    m_name = aName;

    SetVisible( false );
    SetLayer( LAYER_FIELDS );
}


SCH_FIELD::~SCH_FIELD()
{
}


EDA_ITEM* SCH_FIELD::Clone() const
{
    return new SCH_FIELD( *this );
}


wxString SCH_FIELD::GetShownText() const
{
    auto symbolResolver = [ this ]( wxString* token ) -> bool
                          {
                              SCH_COMPONENT* component = static_cast<SCH_COMPONENT*>( m_Parent );
                              std::vector<SCH_FIELD>& fields = component->GetFields();

                              for( int i = 0; i < MANDATORY_FIELDS; ++i )
                              {
                                  if( token->IsSameAs( fields[i].GetCanonicalName().Upper() ) )
                                  {
                                      // silently drop recursive references
                                      if( &fields[i] == this )
                                          *token = wxEmptyString;
                                      else
                                          *token = fields[i].GetShownText();

                                      return true;
                                  }
                              }

                              for( int i = MANDATORY_FIELDS; i < fields.size(); ++i )
                              {
                                  if( token->IsSameAs( fields[i].GetName() )
                                        || token->IsSameAs( fields[i].GetName().Upper() ) )
                                  {
                                      // silently drop recursive references
                                      if( &fields[i] == this )
                                          *token = wxEmptyString;
                                      else
                                          *token = fields[i].GetShownText();

                                      return true;
                                  }
                              }

                              if( token->IsSameAs( wxT( "FOOTPRINT_LIBRARY" ) )  )
                              {
                                  SCH_FIELD& f = component->GetFields()[ FOOTPRINT ];
                                  wxArrayString parts = wxSplit( f.GetText(), ':' );

                                  *token = parts[0];
                                  return true;
                              }
                              else if( token->IsSameAs( wxT( "FOOTPRINT_NAME" ) ) )
                              {
                                  SCH_FIELD& f = component->GetFields()[ FOOTPRINT ];
                                  wxArrayString parts = wxSplit( f.GetText(), ':' );

                                  *token = parts[ std::min( 1, (int) parts.size() - 1 ) ];
                                  return true;
                              }
                              else if( token->IsSameAs( wxT( "UNIT" ) ) )
                              {
                                  *token = LIB_PART::SubReference( component->GetUnit() );
                                  return true;
                              }

                              return false;
                          };

    auto sheetResolver = [ & ]( wxString* token ) -> bool
                         {
                             SCH_SHEET* sheet = static_cast<SCH_SHEET*>( m_Parent );
                             std::vector<SCH_FIELD>& fields = sheet->GetFields();

                             for( int i = 0; i < SHEET_MANDATORY_FIELDS; ++i )
                             {
                                 if( token->IsSameAs( fields[i].GetCanonicalName().Upper() ) )
                                 {
                                     // silently drop recursive references
                                     if( &fields[i] == this )
                                         *token = wxEmptyString;
                                     else
                                         *token = fields[i].GetShownText();

                                     return true;
                                 }
                             }

                             for( int i = SHEET_MANDATORY_FIELDS; i < fields.size(); ++i )
                             {
                                 if( token->IsSameAs( fields[i].GetName() ) )
                                 {
                                     // silently drop recursive references
                                     if( &fields[i] == this )
                                         *token = wxEmptyString;
                                     else
                                         *token = fields[i].GetShownText();

                                     return true;
                                 }
                             }

                             return false;
                         };

    PROJECT*  project = nullptr;
    wxString  text;

    if( g_RootSheet && g_RootSheet->GetScreen() )
        project = &g_RootSheet->GetScreen()->Kiway().Prj();

    if( m_Parent && m_Parent->Type() == SCH_COMPONENT_T )
        text = ExpandTextVars( GetText(), symbolResolver, project );
    else if( m_Parent && m_Parent->Type() == SCH_SHEET_T )
        text = ExpandTextVars( GetText(), sheetResolver, project );
    else
        text = GetText();

    // WARNING: the IDs of FIELDS and SHEETS overlap, so one must check *both* the
    // id and the parent's type.

    if( m_Parent && m_Parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* component = static_cast<SCH_COMPONENT*>( m_Parent );

        if( m_id == REFERENCE )
        {
            // For more than one part per package, we must add the part selection
            // A, B, ... or 1, 2, .. to the reference.
            if( component->GetUnitCount() > 1 )
                text << LIB_PART::SubReference( component->GetUnit() );
        }
    }

    if( m_Parent && m_Parent->Type() == SCH_SHEET_T )
    {
        if( m_id == SHEETFILENAME )
            text = _( "File: " ) + text;
    }

    return text;
}


int SCH_FIELD::GetPenSize() const
{
    int pensize = GetThickness();

    if( pensize == 0 )   // Use default values for pen size
    {
        if( IsBold()  )
            pensize = GetPenSizeForBold( GetTextWidth() );
        else
            pensize = GetDefaultLineThickness();
    }

    // Clip pen size for small texts:
    pensize = Clamp_Text_PenSize( pensize, GetTextSize(), IsBold() );
    return pensize;
}


void SCH_FIELD::Print( wxDC* aDC, const wxPoint& aOffset )
{
    int            orient;
    COLOR4D        color;
    wxPoint        textpos;
    int            lineWidth = GetThickness();

    if( lineWidth == 0 )   // Use default values for pen size
    {
        if( IsBold() )
            lineWidth = GetPenSizeForBold( GetTextWidth() );
        else
            lineWidth = GetDefaultLineThickness();
    }

    // Clip pen size for small texts:
    lineWidth = Clamp_Text_PenSize( lineWidth, GetTextSize(), IsBold() );

    if( ( !IsVisible() && !m_forceVisible) || IsVoid() )
        return;

    // Calculate the text orientation according to the component orientation.
    orient = GetTextAngle();

    if( m_Parent && m_Parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentComponent = static_cast<SCH_COMPONENT*>( m_Parent );

        if( parentComponent && parentComponent->GetTransform().y1 )  // Rotate component 90 degrees.
        {
            if( orient == TEXT_ANGLE_HORIZ )
                orient = TEXT_ANGLE_VERT;
            else
                orient = TEXT_ANGLE_HORIZ;
        }
    }

    /* Calculate the text justification, according to the component
     * orientation/mirror this is a bit complicated due to cumulative
     * calculations:
     * - numerous cases (mirrored or not, rotation)
     * - the DrawGraphicText function recalculate also H and H justifications
     *      according to the text orientation.
     * - When a component is mirrored, the text is not mirrored and
     *   justifications are complicated to calculate
     * so the more easily way is to use no justifications ( Centered text )
     * and use GetBoundaryBox to know the text coordinate considered as centered
     */
    EDA_RECT boundaryBox = GetBoundingBox();
    textpos = boundaryBox.Centre() + aOffset;

    if( m_forceVisible )
        color = COLOR4D( DARKGRAY );
    else
        color = GetLayerColor( m_Layer );

    GRText( aDC, textpos, color, GetShownText(), orient, GetTextSize(),
            GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, lineWidth, IsItalic(), IsBold() );
}


void SCH_FIELD::ImportValues( const LIB_FIELD& aSource )
{
    SetEffects( aSource );
}


void SCH_FIELD::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( (aItem != NULL) && (aItem->Type() == SCH_FIELD_T),
                 wxT( "Cannot swap field data with invalid item." ) );

    SCH_FIELD* item = (SCH_FIELD*) aItem;

    std::swap( m_Layer, item->m_Layer );
    SwapText( *item );
    SwapEffects( *item );
}


const EDA_RECT SCH_FIELD::GetBoundingBox() const
{
    int linewidth = GetThickness() == 0 ? GetDefaultLineThickness() : GetThickness();

    // We must pass the effective text thickness to GetTextBox
    // when calculating the bounding box
    linewidth = Clamp_Text_PenSize( linewidth, GetTextSize(), IsBold() );

    // Calculate the text bounding box:
    EDA_RECT  rect;
    SCH_FIELD text( *this );    // Make a local copy to change text
                                // because GetBoundingBox() is const
    text.SetText( GetShownText() );
    rect = text.GetTextBox( -1, linewidth, false, GetTextMarkupFlags() );

    // Calculate the bounding box position relative to the parent:
    wxPoint origin = GetParentPosition();
    wxPoint pos = GetTextPos() - origin;
    wxPoint begin = rect.GetOrigin() - origin;
    wxPoint end = rect.GetEnd() - origin;
    RotatePoint( &begin, pos, GetTextAngle() );
    RotatePoint( &end, pos, GetTextAngle() );

    // Now, apply the component transform (mirror/rot)
    TRANSFORM transform;

    if( m_Parent && m_Parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentComponent = static_cast<SCH_COMPONENT*>( m_Parent );

        // Due to the Y axis direction, we must mirror the bounding box,
        // relative to the text position:
        MIRROR( begin.y, pos.y );
        MIRROR( end.y,   pos.y );

        transform = parentComponent->GetTransform();
    }
    else
    {
        transform = TRANSFORM( 1, 0, 0, 1 );  // identity transform
    }

    rect.SetOrigin( transform.TransformCoordinate( begin ) );
    rect.SetEnd( transform.TransformCoordinate( end ) );

    rect.Move( origin );
    rect.Normalize();

    return rect;
}


bool SCH_FIELD::IsHorizJustifyFlipped() const
{
    wxPoint render_center = GetBoundingBox().Centre();
    wxPoint pos = GetPosition();

    switch( GetHorizJustify() )
    {
    case GR_TEXT_HJUSTIFY_LEFT:
        return render_center.x < pos.x;
    case GR_TEXT_HJUSTIFY_RIGHT:
        return render_center.x > pos.x;
    default:
        return false;
    }
}


bool SCH_FIELD::IsVoid() const
{
    return GetText().Len() == 0;
}


bool SCH_FIELD::Matches( wxFindReplaceData& aSearchData, void* aAuxData )
{
    wxString text = GetShownText();
    int      flags = aSearchData.GetFlags();
    bool     searchUserDefinedFields = flags & FR_SEARCH_ALL_FIELDS;
    bool     searchAndReplace = flags & FR_SEARCH_REPLACE;
    bool     replaceReferences = flags & FR_REPLACE_REFERENCES;

    wxLogTrace( traceFindItem, wxT( "    child item " )
                    + GetSelectMenuText( EDA_UNITS::MILLIMETRES ) );

    if( m_Parent && m_Parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentComponent = static_cast<SCH_COMPONENT*>( m_Parent );

        if( !searchUserDefinedFields && m_id >= MANDATORY_FIELDS )
            return false;

        if( searchAndReplace && m_id == REFERENCE && !replaceReferences )
            return false;

        // Take sheet path into account which effects the reference field and the unit for
        // components with multiple parts.
        if( m_id == REFERENCE && aAuxData != NULL )
        {
            text = parentComponent->GetRef( (SCH_SHEET_PATH*) aAuxData );

            if( parentComponent->GetUnitCount() > 1 )
                text << LIB_PART::SubReference( parentComponent->GetUnit() );
        }
    }
    else if( m_Parent && m_Parent->Type() == SCH_SHEET_T )
    {
        if( !searchUserDefinedFields && m_id >= SHEET_MANDATORY_FIELDS )
            return false;
    }

    return SCH_ITEM::Matches( text, aSearchData );
}


bool SCH_FIELD::IsReplaceable() const
{
    if( m_Parent && m_Parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentComponent = static_cast<SCH_COMPONENT*>( m_Parent );

        if( m_id == VALUE )
        {
            LIB_PART* part = parentComponent->GetPartRef().get();

            if( part && part->IsPower() )
                return false;
        }
    }
    else if( m_Parent && m_Parent->Type() == SCH_SHEET_T )
    {
        // See comments in SCH_FIELD::Replace(), below.
        if( m_id == SHEETFILENAME )
            return false;
    }

    return true;
}


bool SCH_FIELD::Replace( wxFindReplaceData& aSearchData, void* aAuxData )
{
    bool isReplaced = false;

    if( m_Parent && m_Parent->Type() == SCH_COMPONENT_T && m_id == REFERENCE )
    {
        SCH_COMPONENT* parentComponent = static_cast<SCH_COMPONENT*>( m_Parent );

        if( m_id == REFERENCE )
        {
            wxCHECK_MSG( aAuxData != NULL, false,
                         wxT( "Cannot replace reference designator without valid sheet path." ) );

            wxCHECK_MSG( aSearchData.GetFlags() & FR_REPLACE_REFERENCES, false,
                         wxT( "Invalid replace symbol reference field call." ) ) ;

            wxString text = parentComponent->GetRef( (SCH_SHEET_PATH*) aAuxData );

            isReplaced = EDA_ITEM::Replace( aSearchData, text );

            if( isReplaced )
                parentComponent->SetRef( (SCH_SHEET_PATH*) aAuxData, text );
        }
        else
        {
            isReplaced = EDA_TEXT::Replace( aSearchData );
        }
    }
    else if( m_Parent && m_Parent->Type() == SCH_SHEET_T )
    {
        isReplaced = EDA_TEXT::Replace( aSearchData );

        if( m_id == SHEETFILENAME && isReplaced )
        {
            // If we allowed this we'd have a bunch of work to do here, including warning
            // about it not being undoable, checking for recursive hierarchies, reloading
            // sheets, etc.  See DIALOG_SCH_SHEET_PROPS::TransferDataFromWindow().
        }
    }

    return isReplaced;
}


void SCH_FIELD::Rotate( wxPoint aPosition )
{
    wxPoint pt = GetTextPos();
    RotatePoint( &pt, aPosition, 900 );
    SetTextPos( pt );
}


wxString SCH_FIELD::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Field %s (%s)" ),
                             GetName(),
                             ShortenedShownText() );
}


wxString SCH_FIELD::GetName( bool aUseDefaultName ) const
{
    if( !m_name.IsEmpty() )
        return m_name;
    else if( aUseDefaultName )
    {
        if( m_Parent && m_Parent->Type() == SCH_COMPONENT_T )
            return TEMPLATE_FIELDNAME::GetDefaultFieldName( m_id );
        else if( m_Parent && m_Parent->Type() == SCH_SHEET_T )
            return SCH_SHEET::GetDefaultFieldName( m_id );
    }

    return wxEmptyString;
}


wxString SCH_FIELD::GetCanonicalName() const
{
    if( m_Parent && m_Parent->Type() == SCH_COMPONENT_T )
    {
        switch( m_id )
        {
        case  REFERENCE: return wxT( "Reference" );
        case  VALUE:     return wxT( "Value" );
        case  FOOTPRINT: return wxT( "Footprint" );
        case  DATASHEET: return wxT( "Datasheet" );
        }
    }
    else if( m_Parent && m_Parent->Type() == SCH_SHEET_T )
    {
        switch( m_id )
        {
        case  SHEETNAME:     return wxT( "Sheetname" );
        case  SHEETFILENAME: return wxT( "Sheetfile" );
        }
    }

    return m_name;
}


BITMAP_DEF SCH_FIELD::GetMenuImage() const
{
    if( m_Parent && m_Parent->Type() == SCH_COMPONENT_T )
    {
        switch( m_id )
        {
        case REFERENCE: return edit_comp_ref_xpm;
        case VALUE:     return edit_comp_value_xpm;
        case FOOTPRINT: return edit_comp_footprint_xpm;
        default:        return edit_text_xpm;
        }
    }

    return edit_text_xpm;
}


bool SCH_FIELD::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    // Do not hit test hidden or empty fields.
    if( !IsVisible() || IsVoid() )
        return false;

    EDA_RECT rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool SCH_FIELD::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    // Do not hit test hidden fields.
    if( !IsVisible() || IsVoid() )
        return false;

    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void SCH_FIELD::Plot( PLOTTER* aPlotter )
{
    COLOR4D color = aPlotter->ColorSettings()->GetColor( GetLayer() );

    if( !IsVisible() )
        return;

    if( IsVoid() )
        return;

    // Calculate the text orientation, according to the component orientation/mirror
    int orient = GetTextAngle();

    if( m_Parent && m_Parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentComponent = static_cast<SCH_COMPONENT*>( m_Parent );

        if( parentComponent->GetTransform().y1 )  // Rotate component 90 deg.
        {
            if( orient == TEXT_ANGLE_HORIZ )
                orient = TEXT_ANGLE_VERT;
            else
                orient = TEXT_ANGLE_HORIZ;
        }
    }

    /*
     * Calculate the text justification, according to the component orientation/mirror
     * this is a bit complicated due to cumulative calculations:
     * - numerous cases (mirrored or not, rotation)
     * - the DrawGraphicText function also recalculates H and H justifications according to the
     *   text orientation.
     * - When a component is mirrored, the text is not mirrored and justifications are
     *   complicated to calculate
     * so the easier way is to use no justifications (centered text) and use GetBoundaryBox to
     * know the text coordinate considered as centered
     */
    EDA_RECT BoundaryBox = GetBoundingBox();
    EDA_TEXT_HJUSTIFY_T hjustify = GR_TEXT_HJUSTIFY_CENTER;
    EDA_TEXT_VJUSTIFY_T vjustify = GR_TEXT_VJUSTIFY_CENTER;
    wxPoint  textpos = BoundaryBox.Centre();

    int      thickness = GetPenSize();

    aPlotter->Text( textpos, color, GetShownText(), orient, GetTextSize(),  hjustify, vjustify,
                    thickness, IsItalic(), IsBold() );
}


void SCH_FIELD::SetPosition( const wxPoint& aPosition )
{
    // Actual positions are calculated by the rotation/mirror transform of the
    // parent component of the field.  The inverse transform is used to calculate
    // the position relative to the parent component.
    if( m_Parent && m_Parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentComponent = static_cast<SCH_COMPONENT*>( m_Parent );
        wxPoint        relativePos = aPosition - parentComponent->GetPosition();

        relativePos = parentComponent->GetTransform().
                            InverseTransform().TransformCoordinate( relativePos );

        SetTextPos( relativePos + parentComponent->GetPosition() );
        return;
    }

    SetTextPos( aPosition );
}


wxPoint SCH_FIELD::GetPosition() const
{
    if( m_Parent && m_Parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentComponent = static_cast<SCH_COMPONENT*>( m_Parent );
        wxPoint        relativePos = GetTextPos() - parentComponent->GetPosition();

        relativePos = parentComponent->GetTransform().TransformCoordinate( relativePos );

        return relativePos + parentComponent->GetPosition();
    }

    return GetTextPos();
}


wxPoint SCH_FIELD::GetParentPosition() const
{
    if( m_Parent && m_Parent->Type() == SCH_COMPONENT_T )
        return static_cast<SCH_COMPONENT*>( m_Parent )->GetPosition();
    else if( m_Parent && m_Parent->Type() == SCH_SHEET_T )
        return static_cast<SCH_SHEET*>( m_Parent )->GetPosition();
    else
        return wxPoint();
}


bool SCH_FIELD::operator <( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    auto field = static_cast<const SCH_FIELD*>( &aItem );

    if( GetId() != field->GetId() )
        return GetId() < field->GetId();

    if( GetText() != field->GetText() )
        return GetText() < field->GetText();

    if( GetLibPosition().x != field->GetLibPosition().x )
        return GetLibPosition().x < field->GetLibPosition().x;

    if( GetLibPosition().y != field->GetLibPosition().y )
        return GetLibPosition().y < field->GetLibPosition().y;

    return GetName() < field->GetName();
}
