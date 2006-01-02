/***************************************************************************
                ComicBook.h - ComicBook class and its children
                             -------------------
    begin                : Mon Sep 29 2003
    copyright            : (C) 2004 by James Athey
    email                : jathey@comcast.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   In addition, as a special exception, the author gives permission to   *
 *   link the code of his release of Comical with Rarlabs' "unrar"         *
 *   library (or with modified versions of it that use the same license    *
 *   as the "unrar" library), and distribute the linked executables. You   *
 *   must obey the GNU General Public License in all respects for all of   *
 *   the code used other than "unrar". If you modify this file, you may    *
 *   extend this exception to your version of the file, but you are not    *
 *   obligated to do so. If you do not wish to do so, delete this          *
 *   exception statement from your version.                                *
 *                                                                         *
 ***************************************************************************/

#ifndef _ComicBook_h_
#define _ComicBook_h_

#include <wx/bitmap.h>
#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/log.h>
#include <wx/datetime.h>
#include <wx/utils.h>
#include <wx/stream.h>
#include <wx/thread.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/timer.h>
#include <wx/config.h>
#include <wx/arrstr.h>
#include <wx/event.h>

#include "Resize.h"

enum COMICAL_MODE {ONEPAGE, TWOPAGE};
enum COMICAL_ZOOM {FIT, FITHEIGHT, FITWIDTH, FULL, THREEQ, HALF, ONEQ};
enum COMICAL_ROTATE {NORTH = 0, EAST, SOUTH, WEST};

class ComicBook : public wxThread {

public:

	// Constructors / Destructors
	ComicBook(wxString file);
	virtual ~ComicBook();
  
	// wxThread required functions
	virtual void * Entry();

	void RotatePage(wxUint32 pagenumber, COMICAL_ROTATE direction);
	wxUint32 GetPageCount() { return pageCount; }
	bool SetParams(COMICAL_MODE newMode, FREE_IMAGE_FILTER newFilter, COMICAL_ZOOM newZoom, wxInt32 newWidth, wxInt32 newHeight, wxInt32 newScrollBarThickness);
	wxUint32 GetCacheLen() { return cacheLen; }
	void SetCacheLen(wxUint32 newCacheLen) { cacheLen = newCacheLen; }
	wxBitmap *GetPage(wxUint32 pagenumber);
	wxBitmap *GetPageLeftHalf(wxUint32 pagenumber);
	wxBitmap *GetPageRightHalf(wxUint32 pagenumber);
	wxBitmap *GetThumbnail(wxUint32 pagenumber);
	bool IsPageLandscape(wxUint32 pagenumber);
	wxArrayString *Filenames;
	bool IsPageReady(wxUint32 pagenumber);
	bool IsThumbReady(wxUint32 pagenumber);
	
	wxUint32 GetCurrentPage() { return currentPage; }
	void SetCurrentPage(wxUint32 pagenumber);

	// Instead of insane multiple inheritance, put the evtHandler inside this
	// class and provide passthroughs to its methods
	wxEvtHandler * GetEventHandler() { return evtHandler; }
	void Connect(int id, int lastId, wxEventType eventType, wxObjectEventFunction function, wxObject* userData = NULL, wxEvtHandler* eventSink = NULL)
		{ evtHandler->Connect(id, lastId, eventType, function, userData, eventSink); }
	void Connect(int id, wxEventType eventType, wxObjectEventFunction function, wxObject* userData = NULL, wxEvtHandler* eventSink = NULL)
		{ evtHandler->Connect(id, eventType, function, userData, eventSink); }
	void Connect(wxEventType eventType, wxObjectEventFunction function, wxObject* userData = NULL, wxEvtHandler* eventSink = NULL)
		{ evtHandler->Connect(eventType, function, userData, eventSink); }
	bool Disconnect(wxEventType eventType = wxEVT_NULL, wxObjectEventFunction function = NULL, wxObject* userData = NULL, wxEvtHandler* eventSink = NULL)
		{ return evtHandler->Disconnect(eventType, function, userData, eventSink); }
	bool Disconnect(int id = wxID_ANY, wxEventType eventType = wxEVT_NULL, wxObjectEventFunction function = NULL, wxObject* userData = NULL, wxEvtHandler* eventSink = NULL)
		{ return evtHandler->Disconnect(id, eventType, function, userData, eventSink); }
	bool Disconnect(int id, int lastId = wxID_ANY, wxEventType eventType = wxEVT_NULL, wxObjectEventFunction function = NULL, wxObject* userData = NULL, wxEvtHandler* eventSink = NULL)
		{ return evtHandler->Disconnect(id, lastId, eventType, function, userData, eventSink); }

	COMICAL_ROTATE *Orientations;
	
protected:
	virtual wxInputStream * ExtractStream(wxUint32 pageindex) = 0;
	
	void ScaleImage(wxUint32 pagenumber);
	void ScaleThumbnail(wxUint32 pagenumber);
	// Returns true if the page can fit well in the current zoom mode, i.e., if
	// squeezing the page into the frame doesn't leave more than
	// scrollBarThickness pixels of black space on any side.  Returns the
	// scalingFactor to make the page fit without scrollbars in the parameter.
	bool FitWithoutScrollbars(wxUint32 pagenumber, float *scalingFactor);
	bool FitWithoutScrollbars(wxUint32 pagenumber);
	
	void SendScaledEvent(wxUint32 pagenumber);
	void SendThumbnailedEvent(wxUint32 pagenumber);
	void SendCurrentPageChangedEvent();

	wxUint32 pageCount;
	
	/* Used to prefetch nearby pages and discard distant pages. 
	 * when mode = TWOPAGE, currentPage is the pagenumber of the page on the left.
	 * when mode = ONEPAGE, currentPage is the pagenumber of the displayed page. */
	wxUint32 currentPage;
	wxString filename;
	wxImage *originals;
	wxImage *resamples;
	wxImage *thumbnails;
	
	wxMutex *resampleLockers;
	wxMutex *thumbnailLockers;
	
	wxUint32 cacheLen;
	
	wxInt32 scrollBarThickness;
	
	// window parameters
	COMICAL_MODE mode;
	FREE_IMAGE_FILTER fiFilter;
	COMICAL_ZOOM zoom;
	wxInt32 canvasWidth;
	wxInt32 canvasHeight;

	wxEvtHandler *evtHandler;
};

enum { ID_PageThumbnailed, ID_PageScaled };

DECLARE_EVENT_TYPE(EVT_PAGE_SCALED, -1)
DECLARE_EVENT_TYPE(EVT_PAGE_THUMBNAILED, -1)
DECLARE_EVENT_TYPE(EVT_CURRENT_PAGE_CHANGED, -1)

#endif
