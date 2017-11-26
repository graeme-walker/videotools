//
// Copyright (C) 2017 Graeme Walker
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ===
//
// gxwindow.cpp
//

#include "gdef.h"
#include "gxdef.h"
#include "gxwindow.h"
#include "gxdisplay.h"
#include "gxcolourmap.h"
#include "gxerror.h"
#include "gxwindowmap.h"
#include "gassert.h"

GX::Window::Window( GX::Display & display , int dx , int dy ) :
	Drawable(display) ,
	m_display(display) ,
	m_dx(dx) ,
	m_dy(dy)
{
	unsigned long value_mask = 0UL ;

	if( GX::WindowMap::instance() ) GX::WindowMap::instance()->add( *this ) ;

	XSetWindowAttributes values ;
	values.background_pixmap = None ;
	values.border_pixmap = None ;
	create( display , dx , dy , value_mask , values ) ;
}

GX::Window::Window( GX::Display & display , int dx , int dy , int background , const std::string & title ) :
	Drawable(display) ,
	m_display(display) ,
	m_dx(dx) ,
	m_dy(dy)
{
	unsigned long value_mask = CWBackPixel ;

	XSetWindowAttributes values ;
	values.background_pixel = static_cast<unsigned int>(background) ;
	values.background_pixmap = None ;
	values.border_pixmap = None ;
	create( display , dx , dy , value_mask , values ) ;

	if( !title.empty() )
	{
		XTextProperty text ;
		char * list[1] = { const_cast<char*>(title.c_str()) } ;
		XStringListToTextProperty( list , 1 , &text ) ;
		XSetWMName( display.x() , x() , &text ) ;
		XFree( text.value ) ;
	}

	if( GX::WindowMap::instance() ) GX::WindowMap::instance()->add( *this ) ;
}

void GX::Window::create( GX::Display & display , int dx , int dy ,
	unsigned long value_mask , const XSetWindowAttributes & values )
{
	::Window parent = DefaultRootWindow( display.x() ) ;
	unsigned int border_width = 0U ;
	int depth = CopyFromParent ;
	unsigned int class_ = CopyFromParent ;
	::Visual * visual = nullptr ;
	XSetWindowAttributes * values_p = const_cast<XSetWindowAttributes*>(&values) ;

	G_ASSERT( dx >= 0 && dy >= 0 ) ;
	m_window = XCreateWindow( display.x() , parent , 0 , 0 , 
		static_cast<unsigned int>(dx) , static_cast<unsigned int>(dy) , 
		border_width , depth , class_ , visual , value_mask , values_p ) ;
}

void GX::Window::onKeyPress( XKeyEvent & e )
{
	// try XLoopupString first and use its results iff simple ascii
	{
		m_key.clear() ;
		KeySym keysym = 0 ;
		char buffer[10] = { '\0' } ;
		int rc = XLookupString( &e , buffer , sizeof(buffer) , &keysym , nullptr ) ;
		if( rc == 1 && ( buffer[0] == '\t' || ( buffer[0] >= 0x20 && buffer[0] < 0x7f ) ) )
			m_key = std::string(1U,buffer[0]) ;
	}

	if( m_key.empty() )
		m_key = keysymToString( XLookupKeysym(&e,0) , e.state & ShiftMask ) ;

	if( !m_key.empty() )
		onKey( m_key ) ;
	if( m_key.length() == 1U )
		onChar( m_key.at(0U) ) ;
}

void GX::Window::onKey( const std::string & )
{
}

void GX::Window::onChar( char )
{
}

std::string GX::Window::key() const
{
	return m_key ;
}

GX::Display & GX::Window::windowDisplay()
{
	return m_display ;
}

const GX::Display & GX::Window::windowDisplay() const
{
	return m_display ;
}

void GX::Window::show()
{
	XMapWindow( m_display.x() , m_window ) ;
}

GX::Window::~Window()
{
	try
	{
		if( GX::WindowMap::instance() ) GX::WindowMap::instance()->remove( *this ) ;
		XDestroyWindow( m_display.x() , m_window ) ; // (does an unmap)
	}
	catch(...)
	{
	}
}

::Window GX::Window::x()
{
	return m_window ;
}

::Drawable GX::Window::xd()
{
	return m_window ; // ::Drawable and ::Window are both typedefs for XID
}

void GX::Window::enableEvents( long mask )
{
	XSelectInput( m_display.x() , m_window , mask ) ;
}

long GX::Window::events( bool with_mouse_moves )
{
	return
		(with_mouse_moves?PointerMotionMask:0L) |
		ExposureMask |
		StructureNotifyMask | 
		KeyPressMask | 
		KeyReleaseMask | 
		ButtonPressMask | 
		ButtonReleaseMask ;
}

void GX::Window::clear()
{
	XClearWindow( m_display.x() , m_window ) ;
}

void GX::Window::install( GX::ColourMap & map )
{
	XSetWindowColormap( m_display.x() , m_window , map.x() ) ;
}

int GX::Window::dx() const
{
	return m_dx ;
}

int GX::Window::dy() const
{
	return m_dy ;
}

void GX::Window::sendUserEvent()
{
	static XClientMessageEvent z ;
	XClientMessageEvent event = z ;
	event.type = ClientMessage ;
	event.display = m_display.x() ;
	event.window = m_window ;
	event.message_type = 0 ; // ignored
	event.format = 16 ; // ie. s[]
	event.data.s[0] = 123 ; // ignored
	XSendEvent( m_display.x() , m_window , False , 0 , reinterpret_cast<XEvent*>(&event) ) ;
}

void GX::Window::invalidate()
{
	static XExposeEvent z ;
	XExposeEvent event = z ;
	event.type = Expose ;
	event.display = m_display.x() ;
	event.window = m_window ;
	event.x = 0 ;
	event.y = 0 ;
	event.width = dx() ;
	event.height = dy() ;
	event.count = 0 ;
	XSendEvent( m_display.x() , m_window , False , 0 , reinterpret_cast<XEvent*>(&event) ) ;
}

GX::Capture * GX::Window::capture()
{
	return new GX::Capture( m_display.x() , m_window ) ;
}

std::string GX::Window::keysymToString( ::KeySym keysym , bool shifted )
{
	const char * p = "" ;
	switch( keysym )
	{
		case XK_BackSpace: p = "BackSpace" ; break ;
		case XK_Tab: p = "Tab" ; break ;
		case XK_Linefeed: p = "Linefeed" ; break ;
		case XK_Clear: p = "Clear" ; break ;
		case XK_Return: p = "Return" ; break ;
		case XK_Pause: p = "Pause" ; break ;
		case XK_Scroll_Lock: p = "Scroll_Lock" ; break ;
		case XK_Sys_Req: p = "Sys_Req" ; break ;
		case XK_Escape: p = "Escape" ; break ;
		case XK_Delete: p = "Delete" ; break ;
		case XK_Home: p = "Home" ; break ;
		case XK_Left: p = "Left" ; break ;
		case XK_Up: p = "Up" ; break ;
		case XK_Right: p = "Right" ; break ;
		case XK_Down: p = "Down" ; break ;
		case XK_Page_Up: p = "Page_Up" ; break ;
		case XK_Page_Down: p = "Page_Down" ; break ;
		case XK_End: p = "End" ; break ;
		case XK_Begin: p = "Begin" ; break ;
		case XK_Select: p = "Select" ; break ;
		case XK_Print: p = "Print" ; break ;
		case XK_Execute: p = "Execute" ; break ;
		case XK_Insert: p = "Insert" ; break ;
		case XK_Undo: p = "Undo" ; break ;
		case XK_Redo: p = "Redo" ; break ;
		case XK_Menu: p = "Menu" ; break ;
		case XK_Find: p = "Find" ; break ;
		case XK_Cancel: p = "Cancel" ; break ;
		case XK_Help: p = "Help" ; break ;
		case XK_Break: p = "Break" ; break ;
		case XK_Mode_switch: p = "Mode_switch" ; break ;
		case XK_Num_Lock: p = "Num_Lock" ; break ;
		case XK_KP_Space: p = "KP_Space" ; break ;
		case XK_KP_Tab: p = "KP_Tab" ; break ;
		case XK_KP_Enter: p = "KP_Enter" ; break ;
		case XK_KP_F1: p = "KP_F1" ; break ;
		case XK_KP_F2: p = "KP_F2" ; break ;
		case XK_KP_F3: p = "KP_F3" ; break ;
		case XK_KP_F4: p = "KP_F4" ; break ;
		case XK_KP_Home: p = "KP_Home" ; break ;
		case XK_KP_Left: p = "KP_Left" ; break ;
		case XK_KP_Up: p = "KP_Up" ; break ;
		case XK_KP_Right: p = "KP_Right" ; break ;
		case XK_KP_Down: p = "KP_Down" ; break ;
		case XK_KP_Page_Up: p = "KP_Page_Up" ; break ;
		case XK_KP_Page_Down: p = "KP_Page_Down" ; break ;
		case XK_KP_End: p = "KP_End" ; break ;
		case XK_KP_Begin: p = "KP_Begin" ; break ;
		case XK_KP_Insert: p = "KP_Insert" ; break ;
		case XK_KP_Delete: p = "KP_Delete" ; break ;
		case XK_KP_Equal: p = "KP_Equal" ; break ;
		case XK_KP_Multiply: p = "KP_Multiply" ; break ;
		case XK_KP_Add: p = "KP_Add" ; break ;
		case XK_KP_Separator: p = "KP_Separator" ; break ;
		case XK_KP_Subtract: p = "KP_Subtract" ; break ;
		case XK_KP_Decimal: p = "KP_Decimal" ; break ;
		case XK_KP_Divide: p = "KP_Divide" ; break ;
		case XK_KP_0: p = "KP_0" ; break ;
		case XK_KP_1: p = "KP_1" ; break ;
		case XK_KP_2: p = "KP_2" ; break ;
		case XK_KP_3: p = "KP_3" ; break ;
		case XK_KP_4: p = "KP_4" ; break ;
		case XK_KP_5: p = "KP_5" ; break ;
		case XK_KP_6: p = "KP_6" ; break ;
		case XK_KP_7: p = "KP_7" ; break ;
		case XK_KP_8: p = "KP_8" ; break ;
		case XK_KP_9: p = "KP_9" ; break ;
		case XK_F1: p = "F1" ; break ;
		case XK_F2: p = "F2" ; break ;
		case XK_F3: p = "F3" ; break ;
		case XK_F4: p = "F4" ; break ;
		case XK_F5: p = "F5" ; break ;
		case XK_F6: p = "F6" ; break ;
		case XK_F7: p = "F7" ; break ;
		case XK_F8: p = "F8" ; break ;
		case XK_F9: p = "F9" ; break ;
		case XK_F10: p = "F10" ; break ;
		case XK_F11: p = "F11" ; break ;
		case XK_F12: p = "F12" ; break ;
		case XK_Shift_L: p = "Shift_L" ; break ;
		case XK_Shift_R: p = "Shift_R" ; break ;
		case XK_Control_L: p = "Control_L" ; break ;
		case XK_Control_R: p = "Control_R" ; break ;
		case XK_Caps_Lock: p = "Caps_Lock" ; break ;
		case XK_Shift_Lock: p = "Shift_Lock" ; break ;
		case XK_Meta_L: p = "Meta_L" ; break ;
		case XK_Meta_R: p = "Meta_R" ; break ;
		case XK_Alt_L: p = "Alt_L" ; break ;
		case XK_Alt_R: p = "Alt_R" ; break ;
		case XK_Super_L: p = "Super_L" ; break ;
		case XK_Super_R: p = "Super_R" ; break ;
		case XK_Hyper_L: p = "Hyper_L" ; break ;
		case XK_Hyper_R: p = "Hyper_R" ; break ;
		case XK_space: p = " " ; break ;
		case XK_exclam: p = "!" ; break ;
		case XK_quotedbl: p = "\"" ; break ;
		case XK_numbersign: p = "#" ; break ;
		case XK_dollar: p = "$" ; break ;
		case XK_percent: p = "%" ; break ;
		case XK_ampersand: p = "&" ; break ;
		case XK_apostrophe: p = "'" ; break ;
		case XK_parenleft: p = "(" ; break ;
		case XK_parenright: p = ")" ; break ;
		case XK_asterisk: p = "*" ; break ;
		case XK_plus: p = "+" ; break ;
		case XK_comma: p = shifted ? "<" : "," ; break ;
		case XK_minus: p = shifted ? "_" : "-" ; break ;
		case XK_period: p = shifted ? ">" : "." ; break ;
		case XK_slash: p = shifted ? "?" : "/" ; break ;
		case XK_0: p = shifted ? ")" : "0" ; break ;
		case XK_1: p = shifted ? "!" : "1" ; break ;
		case XK_2: p = shifted ? "\"" : "2" ; break ;
		case XK_3: p = shifted ? "£" : "3" ; break ;
		case XK_4: p = shifted ? "$" : "4" ; break ;
		case XK_5: p = shifted ? "%" : "5" ; break ;
		case XK_6: p = shifted ? "^" : "6" ; break ;
		case XK_7: p = shifted ? "&" : "7" ; break ;
		case XK_8: p = shifted ? "*" : "8" ; break ;
		case XK_9: p = shifted ? "(" : "9" ; break ;
		case XK_colon: p = ":" ; break ;
		case XK_semicolon: p = shifted ? ":" : ";" ;  break ;
		case XK_less: p = "<" ; break ;
		case XK_equal: p = shifted ? "+" : "=" ; break ;
		case XK_greater: p = ">" ; break ;
		case XK_question: p = "?" ; break ;
		case XK_at: p = "@" ; break ;
		case XK_A: p = "A" ; break ;
		case XK_B: p = "B" ; break ;
		case XK_C: p = "C" ; break ;
		case XK_D: p = "D" ; break ;
		case XK_E: p = "E" ; break ;
		case XK_F: p = "F" ; break ;
		case XK_G: p = "G" ; break ;
		case XK_H: p = "H" ; break ;
		case XK_I: p = "I" ; break ;
		case XK_J: p = "J" ; break ;
		case XK_K: p = "K" ; break ;
		case XK_L: p = "L" ; break ;
		case XK_M: p = "M" ; break ;
		case XK_N: p = "N" ; break ;
		case XK_O: p = "O" ; break ;
		case XK_P: p = "P" ; break ;
		case XK_Q: p = "Q" ; break ;
		case XK_R: p = "R" ; break ;
		case XK_S: p = "S" ; break ;
		case XK_T: p = "T" ; break ;
		case XK_U: p = "U" ; break ;
		case XK_V: p = "V" ; break ;
		case XK_W: p = "W" ; break ;
		case XK_X: p = "X" ; break ;
		case XK_Y: p = "Y" ; break ;
		case XK_Z: p = "Z" ; break ;
		case XK_bracketleft: p = shifted ? "{" : "[" ; break ;
		case XK_backslash: p = shifted ? "|" : "\\" ; break ;
		case XK_bracketright: p = shifted ? "}" : "]" ; break ;
		case XK_asciicircum: p = "asciicircum" ; break ;
		case XK_underscore: p = "_" ; break ;
		case XK_grave: p = "`" ; break ;
		case XK_a: p = shifted ? "A" : "a" ; break ;
		case XK_b: p = shifted ? "B" : "b" ; break ;
		case XK_c: p = shifted ? "C" : "c" ; break ;
		case XK_d: p = shifted ? "D" : "d" ; break ;
		case XK_e: p = shifted ? "E" : "e" ; break ;
		case XK_f: p = shifted ? "F" : "f" ; break ;
		case XK_g: p = shifted ? "G" : "g" ; break ;
		case XK_h: p = shifted ? "H" : "h" ; break ;
		case XK_i: p = shifted ? "I" : "i" ; break ;
		case XK_j: p = shifted ? "J" : "j" ; break ;
		case XK_k: p = shifted ? "K" : "k" ; break ;
		case XK_l: p = shifted ? "L" : "l" ; break ;
		case XK_m: p = shifted ? "M" : "m" ; break ;
		case XK_n: p = shifted ? "N" : "n" ; break ;
		case XK_o: p = shifted ? "O" : "o" ; break ;
		case XK_p: p = shifted ? "P" : "p" ; break ;
		case XK_q: p = shifted ? "Q" : "q" ; break ;
		case XK_r: p = shifted ? "R" : "r" ; break ;
		case XK_s: p = shifted ? "S" : "s" ; break ;
		case XK_t: p = shifted ? "T" : "t" ; break ;
		case XK_u: p = shifted ? "U" : "u" ; break ;
		case XK_v: p = shifted ? "V" : "v" ; break ;
		case XK_w: p = shifted ? "W" : "w" ; break ;
		case XK_x: p = shifted ? "X" : "x" ; break ;
		case XK_y: p = shifted ? "Y" : "y" ; break ;
		case XK_z: p = shifted ? "Z" : "z" ; break ;
		case XK_braceleft: p = "{" ; break ;
		case XK_bar: p = "|" ; break ;
		case XK_braceright: p = "}" ; break ;
		case XK_asciitilde: p = "~" ; break ;
		case XK_nobreakspace: p = "nobreakspace" ; break ;
		case XK_exclamdown: p = "exclamdown" ; break ;
		case XK_cent: p = "cent" ; break ;
		case XK_sterling: p = "£" ; break ;
		case XK_currency: p = "currency" ; break ;
		case XK_yen: p = "yen" ; break ;
		case XK_brokenbar: p = "brokenbar" ; break ;
		case XK_section: p = "section" ; break ;
		case XK_diaeresis: p = "diaeresis" ; break ;
		case XK_copyright: p = "copyright" ; break ;
		case XK_ordfeminine: p = "ordfeminine" ; break ;
		case XK_guillemotleft: p = "guillemotleft" ; break ;
		case XK_notsign: p = "notsign" ; break ;
		case XK_hyphen: p = "hyphen" ; break ;
		case XK_registered: p = "registered" ; break ;
		case XK_macron: p = "macron" ; break ;
		case XK_degree: p = "degree" ; break ;
		case XK_plusminus: p = "plusminus" ; break ;
		case XK_twosuperior: p = "twosuperior" ; break ;
		case XK_threesuperior: p = "threesuperior" ; break ;
		case XK_acute: p = "acute" ; break ;
		case XK_mu: p = "mu" ; break ;
		case XK_paragraph: p = "paragraph" ; break ;
		case XK_periodcentered: p = "periodcentered" ; break ;
		case XK_cedilla: p = "cedilla" ; break ;
		case XK_onesuperior: p = "onesuperior" ; break ;
		case XK_masculine: p = "masculine" ; break ;
		case XK_guillemotright: p = "guillemotright" ; break ;
		case XK_onequarter: p = "onequarter" ; break ;
		case XK_onehalf: p = "onehalf" ; break ;
		case XK_threequarters: p = "threequarters" ; break ;
		case XK_questiondown: p = "questiondown" ; break ;
		case XK_Agrave: p = "Agrave" ; break ;
		case XK_Aacute: p = "Aacute" ; break ;
		case XK_Acircumflex: p = "Acircumflex" ; break ;
		case XK_Atilde: p = "Atilde" ; break ;
		case XK_Adiaeresis: p = "Adiaeresis" ; break ;
		case XK_Aring: p = "Aring" ; break ;
		case XK_AE: p = "AE" ; break ;
		case XK_Ccedilla: p = "Ccedilla" ; break ;
		case XK_Egrave: p = "Egrave" ; break ;
		case XK_Eacute: p = "Eacute" ; break ;
		case XK_Ecircumflex: p = "Ecircumflex" ; break ;
		case XK_Ediaeresis: p = "Ediaeresis" ; break ;
		case XK_Igrave: p = "Igrave" ; break ;
		case XK_Iacute: p = "Iacute" ; break ;
		case XK_Icircumflex: p = "Icircumflex" ; break ;
		case XK_Idiaeresis: p = "Idiaeresis" ; break ;
		case XK_ETH: p = "ETH" ; break ;
		case XK_Ntilde: p = "Ntilde" ; break ;
		case XK_Ograve: p = "Ograve" ; break ;
		case XK_Oacute: p = "Oacute" ; break ;
		case XK_Ocircumflex: p = "Ocircumflex" ; break ;
		case XK_Otilde: p = "Otilde" ; break ;
		case XK_Odiaeresis: p = "Odiaeresis" ; break ;
		case XK_multiply: p = "multiply" ; break ;
		//case XK_Ooblique: p = "Ooblique" ; break ;
		case XK_Oslash: p = "Oslash" ; break ;
		case XK_Ugrave: p = "Ugrave" ; break ;
		case XK_Uacute: p = "Uacute" ; break ;
		case XK_Ucircumflex: p = "Ucircumflex" ; break ;
		case XK_Udiaeresis: p = "Udiaeresis" ; break ;
		case XK_Yacute: p = "Yacute" ; break ;
		case XK_THORN: p = "THORN" ; break ;
		case XK_ssharp: p = "ssharp" ; break ;
		case XK_agrave: p = "agrave" ; break ;
		case XK_aacute: p = "aacute" ; break ;
		case XK_acircumflex: p = "acircumflex" ; break ;
		case XK_atilde: p = "atilde" ; break ;
		case XK_adiaeresis: p = "adiaeresis" ; break ;
		case XK_aring: p = "aring" ; break ;
		case XK_ae: p = "ae" ; break ;
		case XK_ccedilla: p = "ccedilla" ; break ;
		case XK_egrave: p = "egrave" ; break ;
		case XK_eacute: p = "eacute" ; break ;
		case XK_ecircumflex: p = "ecircumflex" ; break ;
		case XK_ediaeresis: p = "ediaeresis" ; break ;
		case XK_igrave: p = "igrave" ; break ;
		case XK_iacute: p = "iacute" ; break ;
		case XK_icircumflex: p = "icircumflex" ; break ;
		case XK_idiaeresis: p = "idiaeresis" ; break ;
		case XK_eth: p = "eth" ; break ;
		case XK_ntilde: p = "ntilde" ; break ;
		case XK_ograve: p = "ograve" ; break ;
		case XK_oacute: p = "oacute" ; break ;
		case XK_ocircumflex: p = "ocircumflex" ; break ;
		case XK_otilde: p = "otilde" ; break ;
		case XK_odiaeresis: p = "odiaeresis" ; break ;
		case XK_division: p = "division" ; break ;
		case XK_oslash: p = "oslash" ; break ;
		//case XK_ooblique: p = "ooblique" ; break ;
		case XK_ugrave: p = "ugrave" ; break ;
		case XK_uacute: p = "uacute" ; break ;
		case XK_ucircumflex: p = "ucircumflex" ; break ;
		case XK_udiaeresis: p = "udiaeresis" ; break ;
		case XK_yacute: p = "yacute" ; break ;
		case XK_thorn: p = "thorn" ; break ;
		case XK_ydiaeresis: p = "ydiaeresis" ; break ;
	}
	return std::string(p) ;
}

/// \file gxwindow.cpp
