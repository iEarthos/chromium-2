/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2006 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "RenderThemeWin.h"

#include "Document.h"
#include "FontSelector.h"
#include "NotImplemented.h"

namespace WebCore {

// TODO(port): This is an absolute minimal work needed to get the
// WebCore::theme() call linking. This is guarenteed to give suboptimal
// results, and will need rework when we care about render quality.
class RenderThemeGtk : public RenderTheme {
public:
  // The only abstract method in RenderTheme (and therefore the only thing that
  // needs to be written and for now I'm just making it a notImplemented....).
  //
  // AWESOME TRIVIA NOTE:
  // third_party/WebKit/WebCore/platform/gtk/RenderThemeGtk.cpp also leaves
  // this notImplemented().
  virtual void systemFont(int cssValueId, Document*, FontDescription&) const {
    notImplemented();
  }
};

RenderTheme* theme()
{
    static RenderThemeGtk gtkTheme;
    return &gtkTheme;
}

}
