// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/desktop_media_picker.h"

#include "base/callback.h"
#include "chrome/browser/media/desktop_media_list.h"
#include "chrome/browser/media/desktop_media_list_observer.h"
#include "chrome/browser/ui/ash/ash_util.h"
#include "content/public/browser/browser_thread.h"
#include "grit/generated_resources.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/canvas.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/layout_constants.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_client_view.h"
#include "ui/views/window/dialog_delegate.h"
#include "ui/wm/core/shadow_types.h"

using content::DesktopMediaID;

namespace {

const int kThumbnailWidth = 160;
const int kThumbnailHeight = 100;
const int kThumbnailMargin = 10;
const int kLabelHeight = 40;
const int kListItemWidth = kThumbnailMargin * 2 + kThumbnailWidth;
const int kListItemHeight =
    kThumbnailMargin * 2 + kThumbnailHeight + kLabelHeight;
const int kListColumns = 3;
const int kTotalListWidth = kListColumns * kListItemWidth;

const int kDesktopMediaSourceViewGroupId = 1;

const char kDesktopMediaSourceViewClassName[] =
    "DesktopMediaPicker_DesktopMediaSourceView";

content::DesktopMediaID::Id AcceleratedWidgetToDesktopMediaId(
    gfx::AcceleratedWidget accelerated_widget) {
#if defined(OS_WIN)
  return reinterpret_cast<content::DesktopMediaID::Id>(accelerated_widget);
#else
  return static_cast<content::DesktopMediaID::Id>(accelerated_widget);
#endif
}

class DesktopMediaListView;
class DesktopMediaPickerDialogView;
class DesktopMediaPickerViews;

// View used for each item in DesktopMediaListView. Shows a single desktop media
// source as a thumbnail with the title under it.
class DesktopMediaSourceView : public views::View {
 public:
  DesktopMediaSourceView(DesktopMediaListView* parent,
                         DesktopMediaID source_id);
  virtual ~DesktopMediaSourceView();

  // Updates thumbnail and title from |source|.
  void SetName(const base::string16& name);
  void SetThumbnail(const gfx::ImageSkia& thumbnail);

  // Id for the source shown by this View.
  const DesktopMediaID& source_id() const {
    return source_id_;
  }

  // Returns true if the source is selected.
  bool is_selected() const { return selected_; }

  // Updates selection state of the element. If |selected| is true then also
  // calls SetSelected(false) for the source view that was selected before that
  // (if any).
  void SetSelected(bool selected);

  // views::View interface.
  virtual const char* GetClassName() const OVERRIDE;
  virtual void Layout() OVERRIDE;
  virtual views::View* GetSelectedViewForGroup(int group) OVERRIDE;
  virtual bool IsGroupFocusTraversable() const OVERRIDE;
  virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE;
  virtual void OnFocus() OVERRIDE;
  virtual void OnBlur() OVERRIDE;
  virtual bool OnMousePressed(const ui::MouseEvent& event) OVERRIDE;

 private:
  DesktopMediaListView* parent_;
  DesktopMediaID source_id_;

  views::ImageView* image_view_;
  views::Label* label_;

  bool selected_;

  DISALLOW_COPY_AND_ASSIGN(DesktopMediaSourceView);
};

// View that shows list of desktop media sources available from
// DesktopMediaList.
class DesktopMediaListView : public views::View,
                             public DesktopMediaListObserver {
 public:
  DesktopMediaListView(DesktopMediaPickerDialogView* parent,
                       scoped_ptr<DesktopMediaList> media_list);
  virtual ~DesktopMediaListView();

  void StartUpdating(content::DesktopMediaID::Id dialog_window_id);

  // Called by DesktopMediaSourceView when selection has changed.
  void OnSelectionChanged();

  // Called by DesktopMediaSourceView when a source has been double-clicked.
  void OnDoubleClick();

  // Returns currently selected source.
  DesktopMediaSourceView* GetSelection();

  // views::View overrides.
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual void Layout() OVERRIDE;
  virtual bool OnKeyPressed(const ui::KeyEvent& event) OVERRIDE;

 private:
  // DesktopMediaList::Observer interface
  virtual void OnSourceAdded(int index) OVERRIDE;
  virtual void OnSourceRemoved(int index) OVERRIDE;
  virtual void OnSourceMoved(int old_index, int new_index) OVERRIDE;
  virtual void OnSourceNameChanged(int index) OVERRIDE;
  virtual void OnSourceThumbnailChanged(int index) OVERRIDE;

  DesktopMediaPickerDialogView* parent_;
  scoped_ptr<DesktopMediaList> media_list_;

  DISALLOW_COPY_AND_ASSIGN(DesktopMediaListView);
};

// Dialog view used for DesktopMediaPickerViews.
class DesktopMediaPickerDialogView : public views::DialogDelegateView {
 public:
  DesktopMediaPickerDialogView(gfx::NativeWindow context,
                               gfx::NativeWindow parent_window,
                               DesktopMediaPickerViews* parent,
                               const base::string16& app_name,
                               const base::string16& target_name,
                               scoped_ptr<DesktopMediaList> media_list);
  virtual ~DesktopMediaPickerDialogView();

  // Called by parent (DesktopMediaPickerViews) when it's destroyed.
  void DetachParent();

  // Called by DesktopMediaListView.
  void OnSelectionChanged();
  void OnDoubleClick();

  // views::View overrides.
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual void Layout() OVERRIDE;

  // views::DialogDelegateView overrides.
  virtual base::string16 GetWindowTitle() const OVERRIDE;
  virtual bool IsDialogButtonEnabled(ui::DialogButton button) const OVERRIDE;
  virtual base::string16 GetDialogButtonLabel(
      ui::DialogButton button) const OVERRIDE;
  virtual bool Accept() OVERRIDE;
  virtual void DeleteDelegate() OVERRIDE;

 private:
  DesktopMediaPickerViews* parent_;
  base::string16 app_name_;

  views::Label* label_;
  views::ScrollView* scroll_view_;
  DesktopMediaListView* list_view_;

  DISALLOW_COPY_AND_ASSIGN(DesktopMediaPickerDialogView);
};

// Implementation of DesktopMediaPicker for Views.
class DesktopMediaPickerViews : public DesktopMediaPicker {
 public:
  DesktopMediaPickerViews();
  virtual ~DesktopMediaPickerViews();

  void NotifyDialogResult(DesktopMediaID source);

  // DesktopMediaPicker overrides.
  virtual void Show(gfx::NativeWindow context,
                    gfx::NativeWindow parent,
                    const base::string16& app_name,
                    const base::string16& target_name,
                    scoped_ptr<DesktopMediaList> media_list,
                    const DoneCallback& done_callback) OVERRIDE;

 private:
  DoneCallback callback_;

  // The |dialog_| is owned by the corresponding views::Widget instance.
  // When DesktopMediaPickerViews is destroyed the |dialog_| is destroyed
  // asynchronously by closing the widget.
  DesktopMediaPickerDialogView* dialog_;

  DISALLOW_COPY_AND_ASSIGN(DesktopMediaPickerViews);
};

DesktopMediaSourceView::DesktopMediaSourceView(
    DesktopMediaListView* parent,
    DesktopMediaID source_id)
    : parent_(parent),
      source_id_(source_id),
      image_view_(new views::ImageView()),
      label_(new views::Label()),
      selected_(false)  {
  AddChildView(image_view_);
  AddChildView(label_);
  SetFocusable(true);
}

DesktopMediaSourceView::~DesktopMediaSourceView() {}

void DesktopMediaSourceView::SetName(const base::string16& name) {
  label_->SetText(name);
}

void DesktopMediaSourceView::SetThumbnail(const gfx::ImageSkia& thumbnail) {
  image_view_->SetImage(thumbnail);
}

void DesktopMediaSourceView::SetSelected(bool selected) {
  if (selected == selected_)
    return;
  selected_ = selected;

  if (selected) {
    // Unselect all other sources.
    Views neighbours;
    parent()->GetViewsInGroup(GetGroup(), &neighbours);
    for (Views::iterator i(neighbours.begin()); i != neighbours.end(); ++i) {
      if (*i != this) {
        DCHECK_EQ((*i)->GetClassName(), kDesktopMediaSourceViewClassName);
        DesktopMediaSourceView* source_view =
            static_cast<DesktopMediaSourceView*>(*i);
        source_view->SetSelected(false);
      }
    }

    const SkColor bg_color = GetNativeTheme()->GetSystemColor(
        ui::NativeTheme::kColorId_FocusedMenuItemBackgroundColor);
    set_background(views::Background::CreateSolidBackground(bg_color));

    parent_->OnSelectionChanged();
  } else {
    set_background(NULL);
  }

  SchedulePaint();
}

const char* DesktopMediaSourceView::GetClassName() const {
  return kDesktopMediaSourceViewClassName;
}

void DesktopMediaSourceView::Layout() {
  image_view_->SetBounds(kThumbnailMargin, kThumbnailMargin,
                         kThumbnailWidth, kThumbnailHeight);
  label_->SetBounds(kThumbnailMargin, kThumbnailHeight + kThumbnailMargin,
                    kThumbnailWidth, kLabelHeight);
}

views::View* DesktopMediaSourceView::GetSelectedViewForGroup(int group) {
  Views neighbours;
  parent()->GetViewsInGroup(group, &neighbours);
  if (neighbours.empty())
    return NULL;

  for (Views::iterator i(neighbours.begin()); i != neighbours.end(); ++i) {
    DCHECK_EQ((*i)->GetClassName(), kDesktopMediaSourceViewClassName);
    DesktopMediaSourceView* source_view =
        static_cast<DesktopMediaSourceView*>(*i);
    if (source_view->selected_)
      return source_view;
  }
  return NULL;
}

bool DesktopMediaSourceView::IsGroupFocusTraversable() const {
  return false;
}

void DesktopMediaSourceView::OnPaint(gfx::Canvas* canvas) {
  View::OnPaint(canvas);
  if (HasFocus()) {
    gfx::Rect bounds(GetLocalBounds());
    bounds.Inset(kThumbnailMargin / 2, kThumbnailMargin / 2);
    canvas->DrawFocusRect(bounds);
  }
}

void DesktopMediaSourceView::OnFocus() {
  View::OnFocus();
  SetSelected(true);
  ScrollRectToVisible(gfx::Rect(size()));
  // We paint differently when focused.
  SchedulePaint();
}

void DesktopMediaSourceView::OnBlur() {
  View::OnBlur();
  // We paint differently when focused.
  SchedulePaint();
}

bool DesktopMediaSourceView::OnMousePressed(const ui::MouseEvent& event) {
  if (event.GetClickCount() == 1) {
    RequestFocus();
  } else if (event.GetClickCount() == 2) {
    RequestFocus();
    parent_->OnDoubleClick();
  }
  return true;
}

DesktopMediaListView::DesktopMediaListView(
    DesktopMediaPickerDialogView* parent,
    scoped_ptr<DesktopMediaList> media_list)
    : parent_(parent),
      media_list_(media_list.Pass()) {
  media_list_->SetThumbnailSize(gfx::Size(kThumbnailWidth, kThumbnailHeight));
}

DesktopMediaListView::~DesktopMediaListView() {}

void DesktopMediaListView::StartUpdating(
    content::DesktopMediaID::Id dialog_window_id) {
  media_list_->SetViewDialogWindowId(dialog_window_id);
  media_list_->StartUpdating(this);
}

void DesktopMediaListView::OnSelectionChanged() {
  parent_->OnSelectionChanged();
}

void DesktopMediaListView::OnDoubleClick() {
  parent_->OnDoubleClick();
}

DesktopMediaSourceView* DesktopMediaListView::GetSelection() {
  for (int i = 0; i < child_count(); ++i) {
    DesktopMediaSourceView* source_view =
        static_cast<DesktopMediaSourceView*>(child_at(i));
    DCHECK_EQ(source_view->GetClassName(), kDesktopMediaSourceViewClassName);
    if (source_view->is_selected())
      return source_view;
  }
  return NULL;
}

gfx::Size DesktopMediaListView::GetPreferredSize() {
  int total_rows = (child_count() + kListColumns - 1) / kListColumns;
  return gfx::Size(kTotalListWidth, kListItemHeight * total_rows);
}

void DesktopMediaListView::Layout() {
  int x = 0;
  int y = 0;

  for (int i = 0; i < child_count(); ++i) {
    if (x + kListItemWidth > kTotalListWidth) {
      x = 0;
      y += kListItemHeight;
    }

    View* source_view = child_at(i);
    source_view->SetBounds(x, y, kListItemWidth, kListItemHeight);

    x += kListItemWidth;
  }

  y += kListItemHeight;
  SetSize(gfx::Size(kTotalListWidth, y));
}

bool DesktopMediaListView::OnKeyPressed(const ui::KeyEvent& event) {
  int position_increment = 0;
  switch (event.key_code()) {
    case ui::VKEY_UP:
      position_increment = -kListColumns;
      break;
    case ui::VKEY_DOWN:
      position_increment = kListColumns;
      break;
    case ui::VKEY_LEFT:
      position_increment = -1;
      break;
    case ui::VKEY_RIGHT:
      position_increment = 1;
      break;
    default:
      return false;
  }

  if (position_increment != 0) {
    DesktopMediaSourceView* selected = GetSelection();
    DesktopMediaSourceView* new_selected = NULL;

    if (selected) {
      int index = GetIndexOf(selected);
      int new_index = index + position_increment;
      if (new_index >= child_count())
        new_index = child_count() - 1;
      else if (new_index < 0)
        new_index = 0;
      if (index != new_index) {
        new_selected =
            static_cast<DesktopMediaSourceView*>(child_at(new_index));
      }
    } else if (has_children()) {
      new_selected = static_cast<DesktopMediaSourceView*>(child_at(0));
    }

    if (new_selected) {
      GetFocusManager()->SetFocusedView(new_selected);
    }

    return true;
  }

  return false;
}

void DesktopMediaListView::OnSourceAdded(int index) {
  const DesktopMediaList::Source& source = media_list_->GetSource(index);
  DesktopMediaSourceView* source_view =
      new DesktopMediaSourceView(this, source.id);
  source_view->SetName(source.name);
  source_view->SetGroup(kDesktopMediaSourceViewGroupId);
  AddChildViewAt(source_view, index);

  PreferredSizeChanged();
}

void DesktopMediaListView::OnSourceRemoved(int index) {
  DesktopMediaSourceView* view =
      static_cast<DesktopMediaSourceView*>(child_at(index));
  DCHECK(view);
  DCHECK_EQ(view->GetClassName(), kDesktopMediaSourceViewClassName);
  bool was_selected = view->is_selected();
  RemoveChildView(view);
  delete view;

  if (was_selected)
    OnSelectionChanged();

  PreferredSizeChanged();
}

void DesktopMediaListView::OnSourceMoved(int old_index, int new_index) {
  DesktopMediaSourceView* view =
      static_cast<DesktopMediaSourceView*>(child_at(old_index));
  ReorderChildView(view, new_index);
  PreferredSizeChanged();
}

void DesktopMediaListView::OnSourceNameChanged(int index) {
  const DesktopMediaList::Source& source = media_list_->GetSource(index);
  DesktopMediaSourceView* source_view =
      static_cast<DesktopMediaSourceView*>(child_at(index));
  source_view->SetName(source.name);
}

void DesktopMediaListView::OnSourceThumbnailChanged(int index) {
  const DesktopMediaList::Source& source = media_list_->GetSource(index);
  DesktopMediaSourceView* source_view =
      static_cast<DesktopMediaSourceView*>(child_at(index));
  source_view->SetThumbnail(source.thumbnail);
}

DesktopMediaPickerDialogView::DesktopMediaPickerDialogView(
    gfx::NativeWindow context,
    gfx::NativeWindow parent_window,
    DesktopMediaPickerViews* parent,
    const base::string16& app_name,
    const base::string16& target_name,
    scoped_ptr<DesktopMediaList> media_list)
    : parent_(parent),
      app_name_(app_name),
      label_(new views::Label()),
      scroll_view_(views::ScrollView::CreateScrollViewWithBorder()),
      list_view_(new DesktopMediaListView(this, media_list.Pass())) {
  if (app_name == target_name) {
    label_->SetText(
        l10n_util::GetStringFUTF16(IDS_DESKTOP_MEDIA_PICKER_TEXT, app_name));
  } else {
    label_->SetText(l10n_util::GetStringFUTF16(
        IDS_DESKTOP_MEDIA_PICKER_TEXT_DELEGATED, app_name, target_name));
  }
  label_->SetMultiLine(true);
  label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  AddChildView(label_);

  scroll_view_->SetContents(list_view_);
  AddChildView(scroll_view_);

  DialogDelegate::CreateDialogWidget(this, context, parent_window);

  // DesktopMediaList needs to know the ID of the picker window which
  // matches the ID it gets from the OS. Depending on the OS and configuration
  // we get this ID differently.
  content::DesktopMediaID::Id dialog_window_id = 0;

#if defined(USE_AURA)

#if defined(USE_ASH)
  if (chrome::IsNativeWindowInAsh(GetWidget()->GetNativeWindow())) {
    dialog_window_id = content::DesktopMediaID::RegisterAuraWindow(
        GetWidget()->GetNativeWindow()).id;
  } else
#endif
  {
    dialog_window_id = AcceleratedWidgetToDesktopMediaId(
        GetWidget()->GetNativeWindow()->GetHost()->
            GetAcceleratedWidget());
  }

#else  // defined(USE_AURA)
  dialog_window_id = 0;
  NOTIMPLEMENTED();
#endif  // !defined(USE_AURA)

  list_view_->StartUpdating(dialog_window_id);

  GetWidget()->Show();
}

DesktopMediaPickerDialogView::~DesktopMediaPickerDialogView() {}

void DesktopMediaPickerDialogView::DetachParent() {
  parent_ = NULL;
}

gfx::Size DesktopMediaPickerDialogView::GetPreferredSize() {
  return gfx::Size(600, 500);
}

void DesktopMediaPickerDialogView::Layout() {
  gfx::Rect rect = GetLocalBounds();
  rect.Inset(views::kPanelHorizMargin, views::kPanelVertMargin);

  gfx::Rect label_rect(rect.x(), rect.y(), rect.width(),
                       label_->GetHeightForWidth(rect.width()));
  label_->SetBoundsRect(label_rect);

  int scroll_view_top = label_rect.bottom() + views::kPanelVerticalSpacing;
  scroll_view_->SetBounds(
      rect.x(), scroll_view_top,
      rect.width(), rect.height() - scroll_view_top);
}

base::string16 DesktopMediaPickerDialogView::GetWindowTitle() const {
  return l10n_util::GetStringFUTF16(IDS_DESKTOP_MEDIA_PICKER_TITLE, app_name_);
}

bool DesktopMediaPickerDialogView::IsDialogButtonEnabled(
    ui::DialogButton button) const {
  if (button == ui::DIALOG_BUTTON_OK)
    return list_view_->GetSelection() != NULL;
  return true;
}

base::string16 DesktopMediaPickerDialogView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return l10n_util::GetStringUTF16(button == ui::DIALOG_BUTTON_OK ?
      IDS_DESKTOP_MEDIA_PICKER_SHARE : IDS_CANCEL);
}

bool DesktopMediaPickerDialogView::Accept() {
  DesktopMediaSourceView* selection = list_view_->GetSelection();

  // Ok button should only be enabled when a source is selected.
  DCHECK(selection);

  DesktopMediaID source;
  if (selection)
    source = selection->source_id();

  if (parent_)
    parent_->NotifyDialogResult(source);

  // Return true to close the window.
  return true;
}

void DesktopMediaPickerDialogView::DeleteDelegate() {
  // If the dialog is being closed then notify the parent about it.
  if (parent_)
    parent_->NotifyDialogResult(DesktopMediaID());
  delete this;
}

void DesktopMediaPickerDialogView::OnSelectionChanged() {
  GetDialogClientView()->UpdateDialogButtons();
}

void DesktopMediaPickerDialogView::OnDoubleClick() {
  // This will call Accept() and close the dialog.
  GetDialogClientView()->AcceptWindow();
}

DesktopMediaPickerViews::DesktopMediaPickerViews()
    : dialog_(NULL) {
}

DesktopMediaPickerViews::~DesktopMediaPickerViews() {
  if (dialog_) {
    dialog_->DetachParent();
    dialog_->GetWidget()->Close();
  }
}

void DesktopMediaPickerViews::Show(gfx::NativeWindow context,
                                   gfx::NativeWindow parent,
                                   const base::string16& app_name,
                                   const base::string16& target_name,
                                   scoped_ptr<DesktopMediaList> media_list,
                                   const DoneCallback& done_callback) {
  callback_ = done_callback;
  dialog_ = new DesktopMediaPickerDialogView(
      context, parent, this, app_name, target_name, media_list.Pass());
}

void DesktopMediaPickerViews::NotifyDialogResult(
    DesktopMediaID source) {
  // Once this method is called the |dialog_| will close and destroy itself.
  dialog_->DetachParent();
  dialog_ = NULL;

  DCHECK(!callback_.is_null());

  // Notify the |callback_| asynchronously because it may need to destroy
  // DesktopMediaPicker.
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(callback_, source));
  callback_.Reset();
}

}  // namespace

// static
scoped_ptr<DesktopMediaPicker> DesktopMediaPicker::Create() {
  return scoped_ptr<DesktopMediaPicker>(new DesktopMediaPickerViews());
}
