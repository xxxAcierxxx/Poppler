//========================================================================
//
// This file comes from pdftohtml project
// http://pdftohtml.sourceforge.net
//
// Copyright from:
// Gueorgui Ovtcharov
// Rainer Dorsch <http://www.ra.informatik.uni-stuttgart.de/~rainer/>
// Mikhail Kruk <meshko@cs.brandeis.edu>
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2007, 2010 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2008 Boris Toloknov <tlknv@yandex.ru>
// Copyright (C) 2008 Tomas Are Haavet <tomasare@gmail.com>
// Copyright (C) 2010 OSSD CDAC Mumbai by Leena Chourey (leenac@cdacmumbai.in) and Onkar Potdar (onkar@cdacmumbai.in)
// Copyright (C) 2011 Joshua Richardson <jric@chegg.com>
// Copyright (C) 2011 Stephen Reichling <sreichling@chegg.com>
// Copyright (C) 2012 Igor Slepchin <igor.slepchin@gmail.com>
// Copyright (C) 2012 Luis Parravicini <lparravi@gmail.com>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include "HtmlFonts.h"
#include "HtmlUtils.h"
#include "GlobalParams.h"
#include "UnicodeMap.h"
#include "GfxFont.h"
#include <stdio.h>

 struct Fonts{
    const char *Fontname;
    const char *name;
  };

const int font_num=13;

static Fonts fonts[font_num+1]={  
     {"Courier",               "Courier" },
     {"Courier-Bold",           "Courier"},
     {"Courier-BoldOblique",    "Courier"},
     {"Courier-Oblique",        "Courier"},
     {"Helvetica",              "Helvetica"},
     {"Helvetica-Bold",         "Helvetica"},
     {"Helvetica-BoldOblique",  "Helvetica"},
     {"Helvetica-Oblique",      "Helvetica"},
     {"Symbol",                 "Symbol"   },
     {"Times-Bold",             "Times"    },
     {"Times-BoldItalic",       "Times"    },
     {"Times-Italic",           "Times"    },
     {"Times-Roman",            "Times"    },
     {" "          ,            "Times"    },
};

#define xoutRound(x) ((int)(x + 0.5))
extern GBool xml;
extern GBool fontFullName;

GooString* HtmlFont::DefaultFont=new GooString("Times"); // Arial,Helvetica,sans-serif

HtmlFontColor::HtmlFontColor(GfxRGB rgb){
  r=static_cast<int>(rgb.r/65535.0*255.0);
  g=static_cast<int>(rgb.g/65535.0*255.0);
  b=static_cast<int>(rgb.b/65535.0*255.0);
  if (!(Ok(r)&&Ok(b)&&Ok(g))) {
    if (!globalParams->getErrQuiet()) fprintf(stderr, "Error : Bad color (%d,%d,%d) reset to (0,0,0)\n", r, g, b);
    r=0;g=0;b=0;
  }
}

GooString *HtmlFontColor::convtoX(unsigned int xcol) const{
  GooString *xret=new GooString();
  char tmp;
  unsigned  int k;
  k = (xcol/16);
  if ((k>=0)&&(k<10)) tmp=(char) ('0'+k); else tmp=(char)('a'+k-10);
  xret->append(tmp);
  k = (xcol%16);
  if ((k>=0)&&(k<10)) tmp=(char) ('0'+k); else tmp=(char)('a'+k-10);
  xret->append(tmp);
 return xret;
}

GooString *HtmlFontColor::toString() const{
  GooString *tmp=new GooString("#");
  GooString *tmpr=convtoX(r); 
  GooString *tmpg=convtoX(g);
  GooString *tmpb=convtoX(b);
  tmp->append(tmpr);
  tmp->append(tmpg);
  tmp->append(tmpb);
  delete tmpr;
  delete tmpg;
  delete tmpb;
  return tmp;
} 

HtmlFont::HtmlFont(GfxFont *font, int _size, GfxRGB rgb){
  //if (col) color=HtmlFontColor(col); 
  //else color=HtmlFontColor();
  color=HtmlFontColor(rgb);
  GooString* ftname=font->getName();
  if (!ftname) ftname = getDefaultFont();

  GooString *fontname = NULL;

  if( ftname ){
    fontname = new GooString(ftname);
    FontName=new GooString(ftname);
  }
  else {
    fontname = NULL;
    FontName = NULL;
  }
  
  lineSize = -1;

  size=(_size-1);
  italic = gFalse;
  bold = gFalse;
  rotOrSkewed = gFalse;

  if (font->isBold() || font->getWeight() >= GfxFont::W700) bold=gTrue;
  if (font->isItalic()) italic=gTrue;

  if (fontname){
    if (!bold && strstr(fontname->lowerCase()->getCString(),"bold")) {
		bold=gTrue;
    }

    if (!italic &&
	(strstr(fontname->lowerCase()->getCString(),"italic")||
	 strstr(fontname->lowerCase()->getCString(),"oblique"))) {
		italic=gTrue;
    }

    int i=0;
    while (strcmp(ftname->getCString(),fonts[i].Fontname)&&(i<font_num)) 
	{
		i++;
	}
    pos=i;
    delete fontname;
  } else
    pos = font_num; 
  if (!DefaultFont) DefaultFont=new GooString(fonts[font_num].name);

}
 
HtmlFont::HtmlFont(const HtmlFont& x){
   size=x.size;
   lineSize=x.lineSize;
   italic=x.italic;
   bold=x.bold;
   pos=x.pos;
   color=x.color;
   if (x.FontName) FontName=new GooString(x.FontName);
   rotOrSkewed = x.rotOrSkewed;
   memcpy(rotSkewMat, x.rotSkewMat, sizeof(rotSkewMat));
 }


HtmlFont::~HtmlFont(){
  if (FontName) delete FontName;
}

HtmlFont& HtmlFont::operator=(const HtmlFont& x){
   if (this==&x) return *this; 
   size=x.size;
   lineSize=x.lineSize;
   italic=x.italic;
   bold=x.bold;
   pos=x.pos;
   color=x.color;
   if (FontName) delete FontName;
   if (x.FontName) FontName=new GooString(x.FontName);
   return *this;
}

void HtmlFont::clear(){
  if(DefaultFont) delete DefaultFont;
  DefaultFont = NULL;
}



/*
  This function is used to compare font uniquely for insertion into
  the list of all encountered fonts
*/
GBool HtmlFont::isEqual(const HtmlFont& x) const{
  return (size==x.size) &&
	  (lineSize==x.lineSize) &&
	  (pos==x.pos) && (bold==x.bold) && (italic==x.italic) &&
	  (color.isEqual(x.getColor())) && isRotOrSkewed() == x.isRotOrSkewed() &&
	  (!isRotOrSkewed() || rot_matrices_equal(getRotMat(), x.getRotMat()));
}

/*
  This one is used to decide whether two pieces of text can be joined together
  and therefore we don't care about bold/italics properties
*/
GBool HtmlFont::isEqualIgnoreBold(const HtmlFont& x) const{
  return ((size==x.size) &&
	  (!strcmp(fonts[pos].name, fonts[x.pos].name)) &&
	  (color.isEqual(x.getColor())));
}

GooString* HtmlFont::getFontName(){
   if (pos!=font_num) return new GooString(fonts[pos].name);
    else return new GooString(DefaultFont);
}

GooString* HtmlFont::getFullName(){
  if (FontName)
    return new GooString(FontName);
  else return new GooString(DefaultFont);
} 

void HtmlFont::setDefaultFont(GooString* defaultFont){
  if (DefaultFont) delete DefaultFont;
  DefaultFont=new GooString(defaultFont);
}


GooString* HtmlFont::getDefaultFont(){
  return DefaultFont;
}

// this method if plain wrong todo
GooString* HtmlFont::HtmlFilter(Unicode* u, int uLen) {
  GooString *tmp = new GooString();
  UnicodeMap *uMap;
  char buf[8];
  int n;

  // get the output encoding
  if (!(uMap = globalParams->getTextEncoding())) {
    return tmp;
  }

  for (int i = 0; i < uLen; ++i) {
    switch (u[i])
      { 
	case '"': tmp->append("&#34;");  break;
	case '&': tmp->append("&amp;");  break;
	case '<': tmp->append("&lt;");  break;
	case '>': tmp->append("&gt;");  break;
	case 176: tmp->append("&deg;");  break;
	case 956: tmp->append("&micro;");  break;
	case ' ': tmp->append( !xml && ( i+1 >= uLen || !tmp->getLength() || tmp->getChar( tmp->getLength()-1 ) == ' ' ) ? "&#160;" : " " );
	          break;
	default:  
	  {
	    // convert unicode to string
	    if ((n = uMap->mapUnicode(u[i], buf, sizeof(buf))) > 0) {
	      tmp->append(buf, n); 
	  }
      }
    }
  }

  uMap->decRefCnt();
  return tmp;
}

GooString* HtmlFont::simple(HtmlFont* font, Unicode* content, int uLen){
  GooString *cont=HtmlFilter (content, uLen); 

  /*if (font.isBold()) {
    cont->insert(0,"<b>",3);
    cont->append("</b>",4);
  }
  if (font.isItalic()) {
    cont->insert(0,"<i>",3);
    cont->append("</i>",4);
    } */

  return cont;
}

HtmlFontAccu::HtmlFontAccu(){
  accu=new std::vector<HtmlFont>();
}

HtmlFontAccu::~HtmlFontAccu(){
  if (accu) delete accu;
}

int HtmlFontAccu::AddFont(const HtmlFont& font){
 std::vector<HtmlFont>::iterator i; 
 for (i=accu->begin();i!=accu->end();i++)
 {
	if (font.isEqual(*i)) 
	{
		return (int)(i-(accu->begin()));
	}
 }

 accu->push_back(font);
 return (accu->size()-1);
}

// get CSS font definition for font #i 
GooString* HtmlFontAccu::CSStyle(int i, int j){
   GooString *tmp=new GooString();
   GooString *iStr=GooString::fromInt(i);
   GooString *jStr=GooString::fromInt(j);

   std::vector<HtmlFont>::iterator g=accu->begin();
   g+=i;
   HtmlFont font=*g;
   GooString *Size=GooString::fromInt(font.getSize());
   GooString *colorStr=font.getColor().toString();
   GooString *fontName=(fontFullName ? font.getFullName() : font.getFontName());
   GooString *lSize;
   
   if(!xml){
     tmp->append(".ft");
     tmp->append(jStr);
     tmp->append(iStr);
     tmp->append("{font-size:");
     tmp->append(Size);
     if( font.getLineSize() != -1 && font.getLineSize() != 0 )
     {
	 lSize = GooString::fromInt(font.getLineSize());
	 tmp->append("px;line-height:");
	 tmp->append(lSize);
	 delete lSize;
     }
     tmp->append("px;font-family:");
     tmp->append(fontName); //font.getFontName());
     tmp->append(";color:");
     tmp->append(colorStr);
     // if there is rotation or skew, include the matrix
     if (font.isRotOrSkewed()) {
    	 const double * const text_mat = font.getRotMat();
    	 GooString matrix_str(" matrix(");
    	 matrix_str.appendf("{0:10.10g}, {1:10.10g}, {2:10.10g}, {3:10.10g}, 0, 0)",
    			 text_mat[0], text_mat[1], text_mat[2], text_mat[3]);
    	 tmp->append(";-moz-transform:");
    	 tmp->append(&matrix_str);
    	 tmp->append(";-webkit-transform:");
    	 tmp->append(&matrix_str);
    	 tmp->append(";-o-transform:");
    	 tmp->append(&matrix_str);
    	 tmp->append(";-ms-transform:");
    	 tmp->append(&matrix_str);
    	 // Todo: 75% is a wild guess that seems to work pretty well;
    	 // We probably need to calculate the real percentage
    	 // Based on the characteristic baseline and bounding box of current font
    	 // PDF origin is at baseline
    	 tmp->append(";-moz-transform-origin: left 75%");
    	 tmp->append(";-webkit-transform-origin: left 75%");
    	 tmp->append(";-o-transform-origin: left 75%");
    	 tmp->append(";-ms-transform-origin: left 75%");
     }
     tmp->append(";}");
   }
   if (xml) {
     tmp->append("<fontspec id=\"");
     tmp->append(iStr);
     tmp->append("\" size=\"");
     tmp->append(Size);
     tmp->append("\" family=\"");
     tmp->append(fontName); //font.getFontName());
     tmp->append("\" color=\"");
     tmp->append(colorStr);
     tmp->append("\"/>");
   }

   delete fontName;
   delete colorStr;
   delete jStr;
   delete iStr;
   delete Size;
   return tmp;
}
 

