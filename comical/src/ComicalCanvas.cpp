/***************************************************************************
              ComicalCanvas.cpp - ComicalCanvas implementation
                             -------------------
    begin                : Thu Dec 18 2003
    copyright            : (C) 2003 by James Athey
    email                : jathey@comcast.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ComicalCanvas.h"
#include "ComicBook.h"
#include "Resize.h"

using namespace std;

IMPLEMENT_DYNAMIC_CLASS(ComicalCanvas, wxScrolledWindow)

ComicalCanvas::ComicalCanvas( wxWindow *prnt, wxWindowID id, const wxPoint &pos, const wxSize &size ) : wxScrolledWindow( prnt, id, pos, size, wxSUNKEN_BORDER )
{
  parent = prnt;
  SetBackgroundColour(* wxBLACK);
  scaled = FIT;
  mode = DOUBLE;
  leftPage = rightPage = centerPage = NULL;
  x = -1; y = -1;
  SetFocus(); // This is so we can grab keydown events right away
}

BEGIN_EVENT_TABLE(ComicalCanvas, wxScrolledWindow)
  EVT_PAINT(ComicalCanvas::OnPaint)
  EVT_KEY_DOWN(ComicalCanvas::OnKeyDown)
END_EVENT_TABLE()

ComicalCanvas::~ComicalCanvas()
{
  ClearBitmaps();
  ClearImages();
}

void ComicalCanvas::ClearBitmaps()
{
  if (leftPage) {
    if (leftPage->Ok()) {
      delete leftPage;
      leftPage = NULL;
   }
  }
  if (rightPage) {
    if (rightPage->Ok()) {
      delete rightPage;
      rightPage = NULL;
    }
  }
  if (centerPage) {
    if (centerPage->Ok()) {
      delete centerPage;
      centerPage = NULL;
    }
  }
}

void ComicalCanvas::ClearImages()
{
  if (leftImage.Ok()) leftImage.Destroy();
  if (leftImageScaled.Ok()) leftImageScaled.Destroy();
  if (rightImage.Ok()) rightImage.Destroy();
  if (rightImageScaled.Ok()) rightImageScaled.Destroy();
  if (centerImage.Ok()) centerImage.Destroy();
  if (centerImageScaled.Ok()) centerImageScaled.Destroy();
}

wxImage ComicalCanvas::GetPage(int pagenumber)
{
  wxImage image;
  char *data;
  unsigned int pagecount = theBook->pages.size();

  if (pagecount > 0 && pagenumber >= 0 && pagenumber < (int)pagecount) {

    data = new char[theBook->sizes[pagenumber]];

    wxLogVerbose("Extracting image %i of %i...", pagenumber, (pagecount - 1));
    if (theBook->Extract(pagenumber, data)) {
      wxLogVerbose("Loading data into wxMemoryInputStream...");
      wxMemoryInputStream mis(data, theBook->sizes[pagenumber]);
      if (mis.IsOk()) {
        wxLogVerbose("Loading image from wxMemoryInputStream...");
        if (!image.LoadFile(mis))
	  wxLogError("Successfully extracted the file %s, but could not create an image from it.  It may be corrupted.", theBook->pages[pagenumber].c_str());
      }
      else {
        wxLogError("Extraction succeeded, but could not load the data into a wxMemoryInputStream.");
      }
    }
    else
      wxLogError("Could not extract %s.", theBook->pages[pagenumber].c_str());

    delete[] data;

  }

  return image;

}


void ComicalCanvas::FirstPage()
{
  wxBusyCursor wait;
  wxImage image;
  int xImage, yImage;
  float rImage;

  image = GetPage(0);

  if (image.Ok()) {
    ClearImages();
    xImage = image.GetWidth();
    yImage = image.GetHeight();

    // Here we assume that jpgs that are taller than they are wide are single
    // pages, and jpgs that are wider than they are tall are double pages.
    rImage = float(xImage)/float(yImage);
    if (rImage >= 1.0f || mode == SINGLE) {
      centerImage = image;
      if (scaled != FULL) centerImageScaled = ScaleImage(image);
    }
    else {
      rightImage = image;
      if (scaled != FULL) rightImageScaled = ScaleImage(image);
    }

    theBook->current = 0;
    CreateBitmaps();

  }

  Refresh();

}

void ComicalCanvas::LastPage()
{

  wxBusyCursor wait;
  wxImage image;
  int xImage, yImage;
  float rImage;
  unsigned int pagecount = theBook->pages.size();

  image = GetPage(pagecount - 1);

  if (image.Ok()) {
    ClearImages();
    xImage = image.GetWidth();
    yImage = image.GetHeight();

    // Here we assume that jpgs that are taller than they are wide are single
    // pages, and jpgs that are wider than they are tall are double pages.
    rImage = float(xImage)/float(yImage);
    if (rImage >= 1.0f || mode == SINGLE) {
      if (leftImage.Ok()) leftImage.Destroy();
      if (leftImageScaled.Ok()) leftImageScaled.Destroy();
      centerImage = image;
      if (scaled != FULL) centerImageScaled = ScaleImage(image);
    }
    else {
      if (centerImage.Ok()) centerImage.Destroy();
      if (centerImageScaled.Ok()) centerImageScaled.Destroy();
      leftImage = image;
      if (scaled != FULL) leftImageScaled = ScaleImage(image);
    }

    theBook->current = pagecount - 1;
    CreateBitmaps();

  }

  Refresh();

}

void ComicalCanvas::GoToPage(int pagenumber)
{

  wxBusyCursor wait;
  wxImage image_l, image_r;
  int xImage, yImage;
  float rImage;

  image_l = GetPage(pagenumber);

  if (image_l.Ok()) {
    ClearImages();
    xImage = image_l.GetWidth();
    yImage = image_l.GetHeight();

    // Here we assume that jpgs that are taller than they are wide are single
    // pages, and jpgs that are wider than they are tall are double pages.
    rImage = float(xImage)/float(yImage);
    if (rImage >= 1.0f || mode == SINGLE) {
      centerImage = image_l;
      if (scaled != FULL) centerImageScaled = ScaleImage(image_l);
      theBook->current = pagenumber;
    }
    else {
      leftImage = image_l;
      leftImageScaled = ScaleImage(image_l);
      image_r = GetPage(pagenumber + 1);
      if (image_r.Ok()) {
        xImage = image_r.GetWidth();
        yImage = image_r.GetHeight();
        rImage = float(xImage)/float(yImage);
        if (rImage < 1.0f) { // Only if this page is also not a double do we display it
          rightImage = image_r;
	  if (scaled != FULL) rightImageScaled = ScaleImage(image_r);
          theBook->current = pagenumber + 1;
        }
        else {
          theBook->current = pagenumber;
        }
      }
      else {
        theBook->current = pagenumber;
      }
    }

    CreateBitmaps();

  }

  Refresh();

}

void ComicalCanvas::PrevPageTurn()
{

  wxBusyCursor wait;
  wxImage image_l, image_r;
  int pagenumber, xImage, yImage;
  float rImage;

  if (!leftPage || !rightPage)
    pagenumber = theBook->current - 1;
  else
    pagenumber = theBook->current - 2;

  image_r = GetPage(pagenumber);

  if (image_r.Ok()) {
    ClearImages();
    xImage = image_r.GetWidth();
    yImage = image_r.GetHeight();

    // Here we assume that jpgs that are taller than they are wide are single
    // pages, and jpgs that are wider than they are tall are double pages.

    rImage = float(xImage)/float(yImage);
    if (rImage >= 1.0f || mode == SINGLE) {
      centerImage = image_r;
      if (scaled != FULL) centerImageScaled = ScaleImage(image_r);
      theBook->current = pagenumber;
    }
    else {
      rightImage = image_r;
      rightImageScaled = ScaleImage(image_r);
      image_l = GetPage(pagenumber - 1);
      if (image_l.Ok()) {
        xImage = image_l.GetWidth();
        yImage = image_l.GetHeight();
        rImage = float(xImage)/float(yImage);
        if (rImage < 1.0f) { // Only if this page is also not a double do we display it
          leftImage = image_l;
	  if (scaled != FULL) leftImageScaled = ScaleImage(image_l);
        }
      }
    }

    theBook->current = pagenumber;

    CreateBitmaps();

  }

  Refresh();

}

void ComicalCanvas::NextPageTurn()
{

  wxBusyCursor wait;
  wxImage image_l, image_r;
  int pagenumber, xImage, yImage;
  float rImage;

  pagenumber = theBook->current + 1;

  image_l = GetPage(pagenumber);

  if (image_l.Ok()) {
    ClearImages();
    xImage = image_l.GetWidth();
    yImage = image_l.GetHeight();

    // Here we assume that jpgs that are taller than they are wide are single
    // pages, and jpgs that are wider than they are tall are double pages.
    rImage = float(xImage)/float(yImage);
    if (rImage >= 1.0f || mode == SINGLE) {
      centerImage = image_l;
      if (scaled != FULL) centerImageScaled = ScaleImage(image_l);
      theBook->current = pagenumber;
    }
    else {
      leftImage = image_l;
      leftImageScaled = ScaleImage(image_l);
      image_r = GetPage(pagenumber + 1);
      if (image_r.Ok()) {
        xImage = image_r.GetWidth();
        yImage = image_r.GetHeight();
        rImage = float(xImage)/float(yImage);
        if (rImage < 1.0f) { // Only if this page is also not a double do we display it
          rightImage = image_r;
	  if (scaled != FULL) rightImageScaled = ScaleImage(image_r);
          theBook->current = pagenumber + 1;
        }
        else {
          theBook->current = pagenumber;
        }
      }
      else {
        theBook->current = pagenumber;
      }
    }

    CreateBitmaps();

  }

  Refresh();

}

void ComicalCanvas::PrevPageSlide()
{

  wxBusyCursor wait;
  wxImage image;
  int pagenumber, xImage, yImage;
  float rImage;

  if (centerImage.Ok()) {
	PrevPageTurn();
	return;
  }

  pagenumber = theBook->current - 2;

  image = GetPage(pagenumber);

  if (image.Ok() || mode == SINGLE) {

    if (rightImage.Ok()) rightImage.Destroy();
    if (rightImageScaled.Ok()) rightImageScaled.Destroy();
    if (centerImage.Ok()) centerImage.Destroy();
    if (centerImageScaled.Ok()) centerImageScaled.Destroy();

    xImage = image.GetWidth();
    yImage = image.GetHeight();

    // Here we assume that jpgs that are taller than they are wide are single
    // pages, and jpgs that are wider than they are tall are double pages.
    rImage = float(xImage)/float(yImage);
    if (rImage >= 1.0f) {
      centerImage = image;
      if (scaled != FULL) centerImageScaled = ScaleImage(image);
      if (leftImage.Ok()) leftImage.Destroy();
      if (leftImageScaled.Ok()) leftImageScaled.Destroy();
      theBook->current = pagenumber;
    }
    else {
      if (leftImage.Ok()) {
        xImage = leftImage.GetWidth();
        yImage = leftImage.GetHeight();
        rImage = float(xImage)/float(yImage);
        if (rImage < 1.0f) { // Only if this page is also not a double do we display it
          rightImage = leftImage;
	  if (scaled != FULL) rightImageScaled = leftImageScaled;
        }
      }
      leftImage = image;
      leftImageScaled = ScaleImage(image);
      theBook->current--;
    }

    CreateBitmaps();

  }

  Refresh();

}

void ComicalCanvas::NextPageSlide()
{

  wxBusyCursor wait;
  wxImage image;
  int pagenumber, xImage, yImage;
  float rImage;

  if (centerImage.Ok()) {
	NextPageTurn();
	return;
  }

  pagenumber = theBook->current + 1;

  image = GetPage(pagenumber);

  if (image.Ok()) {

    if (leftImage.Ok()) leftImage.Destroy();
    if (leftImageScaled.Ok()) leftImageScaled.Destroy();
    if (centerImage.Ok()) centerImage.Destroy();
    if (centerImageScaled.Ok()) centerImageScaled.Destroy();

    xImage = image.GetWidth();
    yImage = image.GetHeight();

    // Here we assume that jpgs that are taller than they are wide are single
    // pages, and jpgs that are wider than they are tall are double pages.

    rImage = float(xImage)/float(yImage);
    if (rImage >= 1.0f || mode == SINGLE) {
      centerImage = image;
      if (scaled != FULL) centerImageScaled = ScaleImage(image);
      if (rightImage.Ok()) rightImage.Destroy();
      if (rightImageScaled.Ok()) rightImageScaled.Destroy();
    }
    else {
      if (rightImage.Ok()) {
        xImage = rightImage.GetWidth();
        yImage = rightImage.GetHeight();
        rImage = float(xImage)/float(yImage);
        if (rImage < 1.0f) { // Only if this page is also not a double do we display it
          leftImage = rightImage;
	  if (scaled != FULL) leftImageScaled = rightImageScaled;
        }
      }
      rightImage = image;
      rightImageScaled = ScaleImage(image);
    }

    theBook->current++;

    CreateBitmaps();

  }

  Refresh();

}

void ComicalCanvas::CreateBitmaps()
{

  int xScroll = 0, yScroll = 0, xWindow, yWindow;
  bool left = false, right = false;

  wxLogVerbose("Creating bitmaps from images.");

  GetClientSize(&xWindow, &yWindow);
  ClearBitmaps();

  if (scaled != FULL)  {
    if (centerImageScaled.Ok())
      centerPage = new wxBitmap(centerImageScaled);
    else {
      if ((left = leftImageScaled.Ok()))
        leftPage = new wxBitmap(leftImageScaled);
      if ((right = rightImageScaled.Ok()))
        rightPage = new wxBitmap(rightImageScaled);
    }
  }
  else {
    if (centerImage.Ok())
      centerPage = new wxBitmap(centerImage);
    else {
      if ((left = leftImage.Ok()))
        leftPage = new wxBitmap(leftImage);
      if ((right = rightImage.Ok()))
        rightPage = new wxBitmap(rightImage);
    }
  }

  if (centerPage) {
    if (centerPage->Ok()) {
      xScroll = centerPage->GetWidth();
      yScroll = centerPage->GetHeight();
    }
  }
  else {
    if (leftPage) if (leftPage->Ok()) {
      xScroll += leftPage->GetWidth();
      yScroll = leftPage->GetHeight();
    }
    if (rightPage) if (rightPage->Ok()) {
      xScroll += rightPage->GetWidth();
      yScroll = (rightPage->GetHeight() > yScroll) ? rightPage->GetHeight() : yScroll;
    }
    if (!left || !right) // if only one page is active
      xScroll *= 2;
  }

  SetScrollbars( 10, 10, xScroll / 10, yScroll / 10 );
  Scroll ((xScroll / 20) - (xWindow / 20), 0);

}

void ComicalCanvas::Scale(int value) {

  wxBusyCursor wait;
  scaled = value;
  if (scaled != FULL) {
    if (leftImage.Ok()) leftImageScaled = ScaleImage(leftImage);
    if (rightImage.Ok()) rightImageScaled = ScaleImage(rightImage);
    if (centerImage.Ok()) centerImageScaled = ScaleImage(centerImage);
  }
  CreateBitmaps();
  Refresh();

}

void ComicalCanvas::Mode(int newmode) {

  wxLogVerbose("Mode = %i", newmode);
  switch (newmode) {

  case SINGLE:
  case DOUBLE:

    mode = newmode;
    GoToPage(theBook->current);
    break;

  default:
    wxLogVerbose("Undefined page mode."); // we shouldn't be here
    break;

  }

}

void ComicalCanvas::OnPaint( wxPaintEvent &WXUNUSED(event) )
{

  int xCanvas, yCanvas;

  wxPaintDC dc( this );
  PrepareDC( dc );

  GetVirtualSize(&xCanvas, &yCanvas);

  /* I can't properly initialize x and y until the constructors are done, but
     if I just leave the values uninitialized, the very first image will be
     resized twice.  Here, when OnPaint is called for the first time, the values
     will be initialized. */
  if (x == -1 && y == -1) {
    x = xCanvas;
    y = yCanvas;
  }

  if (scaled == FIT || scaled == FITH || scaled == FITV) {
    if (xCanvas != x && yCanvas != y) {
      x = xCanvas;
      y = yCanvas;
      wxLogVerbose("Window size has changed, rescaling to %i x %i", x, y);
      Scale(scaled);
    }
  }

  if (centerPage) {
    if (centerPage->Ok())
      dc.DrawBitmap(*centerPage, xCanvas/2 - centerPage->GetWidth()/2, 0);
  }
  else {
    if (leftPage) if (leftPage->Ok())
      dc.DrawBitmap(*leftPage, xCanvas/2 - leftPage->GetWidth(), 0);
    if (rightPage) if (rightPage->Ok())
      dc.DrawBitmap(*rightPage, xCanvas/2, 0);
  }

}

/* Resizes an image to fit within half of the Comical window. */
wxImage ComicalCanvas::ScaleImage(wxImage orig) {

  int xCanvas, yCanvas, xImage, yImage;
  float rCanvas, rImage;  // width/height ratios
  float scalingFactor;

  xImage = orig.GetWidth();
  yImage = orig.GetHeight();

  switch(scaled) {

  case THREEQ:
    wxLogVerbose("scalingFactor = 0.75");
    scalingFactor = 0.75f;
    break;

  case HALF:
    wxLogVerbose("scalingFactor = 0.5");
    scalingFactor = 0.5f;
    break;

  case ONEQ:
    wxLogVerbose("scalingFactor = 0.25");
    scalingFactor = 0.25f;
    break;

  case FIT:

    GetClientSize(&xCanvas, &yCanvas);

    rImage = float(xImage) / float(yImage);

    // This seems like an ugly hack to me.  If the centerImage is present,
    // then we assume we're dealing with a double page or we're in single
    // page mode.
    if (centerImage.Ok()) {
      rCanvas = float(xCanvas) / float(yCanvas);
      if (rCanvas > rImage)
        scalingFactor = float(yCanvas) / float(yImage);
      else
        scalingFactor = float(xCanvas) / float(xImage);
    }
    else {
      rCanvas = (float(xCanvas)/2.0f) / float(yCanvas);
      if (rCanvas > rImage)
        scalingFactor = float(yCanvas) / float(yImage);
      else
        scalingFactor = (float(xCanvas)/2.0f) / float(xImage);
    }

    wxLogVerbose("Image:%ix%i, Canvas:%ix%i, rCanvas:%f, rImage:%f, scalingFactor=%f", xImage, yImage, xCanvas, yCanvas, rCanvas, rImage, scalingFactor);
    break;

  case FITH: // fit horizontally

    GetClientSize(&xCanvas, &yCanvas);

    if (centerImage.Ok())
      scalingFactor = float(xCanvas) / float(xImage);
    else
      scalingFactor = (float(xCanvas)/2.0f) / float(xImage);

    wxLogVerbose("scalingFactor = %f", scalingFactor);

    break;

  case FITV: // fit vertically

    GetClientSize(&xCanvas, &yCanvas);
    scalingFactor = float(yCanvas) / float(yImage);
    wxLogVerbose("scalingFactor = %f", scalingFactor);

    break;

  default:
    wxLogVerbose("scalingFactor = %f", scalingFactor);
    return orig;
    break;

  }

  wxLogVerbose("Scaling image with filter %i.", FILTER_LANCZOS3);
  wxImage scaled = FreeImage_Rescale(orig, int(xImage * scalingFactor), int(yImage * scalingFactor), FILTER_LANCZOS3);
  return scaled;

}

void ComicalCanvas::OnKeyDown(wxKeyEvent& event)
{

  switch(event.GetKeyCode()) {

  case WXK_PRIOR:
    PrevPageTurn();
    break;

  case WXK_NEXT:
    NextPageTurn();
    break;

  case WXK_LEFT:
    PrevPageSlide();
    break;

  case WXK_RIGHT:
  case WXK_SPACE:
    NextPageSlide();
    break;

  case WXK_UP:

  case WXK_DOWN:

    break;

  case WXK_HOME:
    FirstPage();
    break;

  case WXK_END:
    LastPage();
    break;

  default:
    event.Skip();
  }

}
