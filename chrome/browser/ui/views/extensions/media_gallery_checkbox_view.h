// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_EXTENSIONS_MEDIA_GALLERY_CHECKBOX_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_EXTENSIONS_MEDIA_GALLERY_CHECKBOX_VIEW_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/gfx/size.h"
#include "ui/views/view.h"

namespace views {
class ButtonListener;
class Checkbox;
class ContextMenuController;
class ImageButton;
class Label;
}  // namespace views

// A view composed of a checkbox, optional folder icon button, and secondary
// text that will elide to its parent's width; used by MediaGalleriesDialogViews
// and MediaGalleriesScanResultDialogViews.
class MediaGalleryCheckboxView : public views::View {
 public:
  MediaGalleryCheckboxView(const base::string16& label,
                           const base::string16& tooltip_text,
                           const base::string16& details,
                           bool show_button,
                           int trailing_vertical_space,
                           views::ButtonListener* button_listener,
                           views::ContextMenuController* menu_controller);
  virtual ~MediaGalleryCheckboxView();

  // Overrides from views::View.
  virtual void Layout() OVERRIDE;

  views::Checkbox* checkbox() { return checkbox_; }
  views::ImageButton* folder_viewer_button() { return folder_viewer_button_; }
  views::Label* secondary_text() { return secondary_text_; }

 private:
  // Owned by the parent class (views::View).
  views::Checkbox* checkbox_;
  views::ImageButton* folder_viewer_button_;
  views::Label* secondary_text_;

  DISALLOW_COPY_AND_ASSIGN(MediaGalleryCheckboxView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_EXTENSIONS_MEDIA_GALLERY_CHECKBOX_VIEW_H_
