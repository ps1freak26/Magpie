#include "pch.h"
#include "PageFrame.h"
#if __has_include("PageFrame.g.cpp")
#include "PageFrame.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;


namespace winrt::Magpie::implementation {

const DependencyProperty PageFrame::TitleProperty = DependencyProperty::Register(
	L"Title",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::PageFrame>(),
	PropertyMetadata(box_value(L""), &PageFrame::_OnTitleChanged)
);

const DependencyProperty PageFrame::MainContentProperty = DependencyProperty::Register(
	L"MainContent",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::PageFrame>(),
	PropertyMetadata(nullptr)
);


PageFrame::PageFrame() {
	InitializeComponent();
}

void PageFrame::Title(const hstring& value) {
	SetValue(TitleProperty, box_value(value));
}

hstring PageFrame::Title() const {
	return GetValue(TitleProperty).as<hstring>();
}

void PageFrame::MainContent(IInspectable const& value) {
	SetValue(MainContentProperty, box_value(value));
}

IInspectable PageFrame::MainContent() const {
	return GetValue(MainContentProperty).as<IInspectable>();
}

void PageFrame::PageFrame_Loading(FrameworkElement const&, IInspectable const&) {
	_Update();
}

void PageFrame::_Update() {
	TitleTextBlock().Visibility(Title().empty() ? Visibility::Collapsed : Visibility::Visible);
}

void PageFrame::_OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args) {
	get_self<PageFrame>(sender.as<default_interface<PageFrame>>())->_Update();
}

}
