#include "NotificationManager.hpp"

#include "GUI_App.hpp"
#include "GLCanvas3D.hpp"
#include "ImGuiWrapper.hpp"

#include "wxExtensions.hpp"

#include <wx/glcanvas.h>
#include <iostream>




#define NOTIFICATION_MAX_MOVE 3.0f

#define GAP_WIDTH 10.0f
#define SPACE_RIGHT_PANEL 10.0f

namespace Slic3r {
namespace GUI {

wxDEFINE_EVENT(EVT_EJECT_DRIVE_NOTIFICAION_CLICKED, EjectDriveNotificationClickedEvent);

//ScalableBitmap bmp_icon;
//------PopNotification--------
NotificationManager::PopNotification::PopNotification(const NotificationData &n, const int id, wxEvtHandler* evt_handler) :
	  m_data          (n)
	, m_id            (id)    
	, m_creation_time (wxGetLocalTime())
	, m_evt_handler   (evt_handler)
{
	//bmp_icon = ScalableBitmap(wxGetApp().plater_, "eject_sd", bmp_px_cnt);
}
//NotificationManager::PopNotification::~PopNotification()
//{}
NotificationManager::PopNotification::RenderResult NotificationManager::PopNotification::render(GLCanvas3D& canvas, const float& initial_x)
{
	if (m_finished)
		return RenderResult::Finished;
	
	if (m_data.duration != 0 && wxGetLocalTime() - m_creation_time >= m_data.duration)
		m_close_pending = true;

	if (m_close_pending) {
		// request of extra frame will be done in caller function by ret val ClosePending
		m_finished = true;
		return RenderResult::ClosePending;
	}

	RenderResult    ret_val = RenderResult::Static;
	Size            cnv_size = canvas.get_canvas_size();
	ImGuiWrapper&   imgui = *wxGetApp().imgui();
	bool            new_target = false;
	bool            shown = true;
	std::string     name;

	//movent
	/*
	if (m_target_x != initial_x + m_window_height)
	{
		m_target_x = initial_x + m_window_height;
		new_target = true;
	}
	if (m_current_x < m_target_x) {
		if (new_target || m_move_step < 1.0f)
			m_move_step = std::min((m_target_x - m_current_x) / 20, NOTIFICATION_MAX_MOVE);
		m_current_x += m_move_step;
		ret_val = RenderResult::Moving;
	}
	if (m_current_x > m_target_x)
		m_current_x = m_target_x;
		*/
	
	//background color
	//ImVec4 backcolor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
	//backcolor.w = 0.5f;
	//ImGui::PushStyleColor(ImGuiCol_WindowBg, backcolor);
	//top of window
	m_target_x = initial_x + m_window_height;
	ImVec2 window_pos(1.0f * (float)cnv_size.get_width() - SPACE_RIGHT_PANEL, 1.0f * (float)cnv_size.get_height() - m_target_x);
	imgui.set_next_window_pos(window_pos.x, window_pos.y, ImGuiCond_Always, 1.0f, 0.0f);
	//set_next_window_size should be calculated with respect to size of all notifications and text
	ImVec2 text1_size = ImGui::CalcTextSize(m_data.text1.c_str());
	int window_height = (text1_size.x > 350 ? 90 : 55);
	imgui.set_next_window_size(450, window_height, ImGuiCond_Always);

	//name of window - probably indentifies window and is shown so i add whitespaces according to id
	for (size_t i = 0; i < m_id; i++)
		name += " ";
	if (imgui.begin(name, &shown, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
		if (shown) {
			ImVec2 win_size = ImGui::GetWindowSize();
			m_window_height = win_size.y;
			m_window_width = win_size.x;
			
			//FIXME: dont forget to us this for texts
			//boost::format(_utf8(L(
			
			//way to set position of next element in window
			//ImGui::SetCursorPosY(30);
			//ImGui::SetCursorPosX(100);
			

			/*
			ImGuiCol_ level_color_tag;
			switch (m_data.level) {
			    case NotificationLevel::RegularNotification: level_color_tag = ImGuiCol_WindowBg; break;
			    case NotificationLevel::ErrorNotification: level_color_tag = ImGuiCol_Header;  break;
			    case NotificationLevel::ImportantNotification: level_color_tag = ImGuiCol_Button; break;
			}
			*/
			//push&pop style color set color (first will heave color of second). pop after push or you get crash!  
			//ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
			//const ImVec4& backcolor = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
			


			//ImGui::GetFont()->FontSize = 35;

			//notification text 1
			std::string fulltext = m_data.text1 + m_data.hypertext + m_data.text2;
			ImVec2 text_size = ImGui::CalcTextSize(fulltext.c_str());
			float cursor_y = win_size.y / 2 - text_size.y / 2;
			if(text1_size.x > 350) { // split in half
				//first half
				cursor_y = win_size.y / 2 - win_size.y / 6 - text_size.y / 2;
				int half = m_data.text1.find_first_of(' ', m_data.text1.length() / 2 - 1);
				std::string first_half_text1 = m_data.text1.substr(0, half);
				std::string second_half_text1 = m_data.text1.substr(half);
				ImVec2 first_half_text1_size = ImGui::CalcTextSize(first_half_text1.c_str());
				ImGui::SetCursorPosX(win_size.x / 2 - first_half_text1_size.x / 2);
				ImGui::SetCursorPosY(cursor_y);
				imgui.text(first_half_text1.c_str());
				//second half
				cursor_y = win_size.y / 2 + win_size.y / 6 - text_size.y / 2;
				fulltext = second_half_text1 + m_data.hypertext + m_data.text2;
				text_size = ImGui::CalcTextSize(fulltext.c_str());
				ImGui::SetCursorPosX(win_size.x / 2 - text_size.x / 2);
				ImGui::SetCursorPosY(cursor_y);
				imgui.text(second_half_text1.c_str());
			} else {
				ImGui::SetCursorPosX(win_size.x / 2 - text_size.x / 2);
				ImGui::SetCursorPosY(cursor_y);
				imgui.text(m_data.text1.c_str());
			}
			
			


			//notification hyperlink text
			if(!m_data.hypertext.empty())
			{
				ImVec2 prev_size = ImGui::CalcTextSize(m_data.text1.c_str());
				ImVec2 part_size = ImGui::CalcTextSize(m_data.hypertext.c_str());
				ImGui::SetCursorPosX(win_size.x / 2 - text_size.x / 2 + prev_size.x);
				ImGui::SetCursorPosY(cursor_y - 5);
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.0f, .0f, .0f, .0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.0f, .0f, .0f, .0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.0f, .0f, .0f, .0f));
				//ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
				//ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 0.7f));
				if (imgui.button("", part_size.x + 6, part_size.y + 10))
				{
					on_text_click();
					m_close_pending = true;
				}
				ImGui::SetCursorPosX(win_size.x / 2 - text_size.x / 2 + prev_size.x + 2);
				ImGui::SetCursorPosY(cursor_y);
				imgui.text(m_data.hypertext.c_str());
				ImGui::PopStyleColor();
				ImGui::PopStyleColor();
				ImGui::PopStyleColor();
				//ImGui::PopStyleColor();

				if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

				//ImVec2 lineStart(window_pos.x - win_size.x + win_size.x / 2 - text_size.x / 2 + prev_size.x, window_pos.y - win_size.y / 2 - part_size.y);
				//ImVec2 lineEnd = lineStart;
				//lineEnd.x += part_size.x;
				
				ImVec2 lineEnd = ImGui::GetItemRectMax();
				lineEnd.y -= 2;
				ImVec2 lineStart = lineEnd;
				lineStart.x = ImGui::GetItemRectMin().x;
				ImVec4 text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
				ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, IM_COL32((int)(text_color.x * 255), (int)(text_color.y * 255), (int)(text_color.z * 255), (int)(text_color.w * 255)));

				
			}

			//notification text 2
			if (!m_data.text2.empty())
			{
				ImVec2 part_size = ImGui::CalcTextSize(m_data.hypertext.c_str());
				ImGui::SetCursorPosX(win_size.x / 2 + text_size.x / 2 - part_size.x + 4);
				ImGui::SetCursorPosY(cursor_y);

				imgui.text(m_data.text2.c_str());
			}
			

			//bool IsItemHovered();      // is the last item hovered by mouse (and usable)? or we are currently using Nav and the item is focused.
			//bool IsItemHoveredRect();  // is the last item hovered by mouse? even if another item is active or window is blocked by popup while we are hovering this
			
			if(ImGui::IsItemHovered())
			{
				ImGui::SetWindowFocus();
			}
			
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.0f, .0f, .0f, .0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));

			//ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.75f, .75f, .75f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0, .75f, .75f, 1.f));

			ImGui::SetCursorPosX(win_size.x - 40);
			ImGui::SetCursorPosY(win_size.y / 2 - 15/*- text_size.y / 2*/);
			//button - if part if treggered
			if (imgui.button("X", 30, 30))
			{
				m_close_pending = true;
			}

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();

#ifdef __APPLE__
			int bmp_px_cnt = 16;
#else
			int bmp_px_cnt = 32;
#endif //__APPLE__
			//ScalableBitmap bmp = ScalableBitmap(wxGetApp().plater_, "eject_sd", bmp_px_cnt);
			//const wxBitmap& wxbmp = bmp_icon.bmp();
			//ImTextureID texture_id;
			//ImGui::Image(,);
			//ImGui::ImageButton();

			
			//ImGui::PopStyleColor();
			
		} else {
			// the user clicked on the [X] button ( ImGuiWindowFlags_NoTitleBar means theres no [X] button)
			m_close_pending = true;
			canvas.set_as_dirty();
		}
	}
	//ImGui::PopStyleColor();
	imgui.end();
	return ret_val;
}
void NotificationManager::PopNotification::on_text_click()
{
	switch (m_data.type)
	{
	case NotificationType::ExportToRemovableFinished :
		assert(m_evt_handler != nullptr);
		if (m_evt_handler != nullptr)
			wxPostEvent(m_evt_handler, EjectDriveNotificationClickedEvent(EVT_EJECT_DRIVE_NOTIFICAION_CLICKED));
		break;
	default:
		break;
	}
}
void NotificationManager::PopNotification::update()
{
	m_creation_time = wxGetLocalTime();
}
//------NotificationManager--------
NotificationManager::NotificationManager(wxEvtHandler* evt_handler) :
	m_evt_handler(evt_handler)
{}
NotificationManager::~NotificationManager()
{
	for (PopNotification* notification : m_pop_notifications)
	{
		delete notification;
	}
}
void NotificationManager::push_notification(const NotificationType type, GLCanvas3D& canvas)
{
	auto it = std::find_if(basic_notifications.begin(), basic_notifications.end(),
		boost::bind(&NotificationData::type, _1) == type);	
	if (it != basic_notifications.end())
		push_notification_data( *it, canvas);
}
void NotificationManager::push_notification(const std::string& text, GLCanvas3D& canvas)
{
	push_notification_data({ NotificationType::CustomNotification, NotificationLevel::RegularNotification, 10, text }, canvas );
}
void NotificationManager::push_notification(const std::string& text, NotificationManager::NotificationLevel level, GLCanvas3D& canvas)
{
	switch (level)
	{
	case Slic3r::GUI::NotificationManager::NotificationLevel::RegularNotification:
		push_notification_data({ NotificationType::CustomNotification, level, 10, text }, canvas);
		break;
	case Slic3r::GUI::NotificationManager::NotificationLevel::ErrorNotification:
		push_notification_data({ NotificationType::CustomNotification, level, 0, text }, canvas);
		break;
	case Slic3r::GUI::NotificationManager::NotificationLevel::ImportantNotification:
		push_notification_data({ NotificationType::CustomNotification, level, 0, text }, canvas);
		break;
	default:
		break;
	}
	
}
void NotificationManager::push_notification_data(const NotificationData &notification_data,  GLCanvas3D& canvas)
{
	if (!this->find_older(notification_data.type))
		m_pop_notifications.emplace_back(new PopNotification(notification_data, m_next_id++, m_evt_handler));
	else
		m_pop_notifications.back()->update();
	//std::cout << "PUSH: " << text << std::endl;
	canvas.request_extra_frame();
}
void NotificationManager::render_notifications(GLCanvas3D& canvas)
{
	float    last_x = 0.0f;
	float    current_height = 0.0f;
	bool     request_next_frame = false;
	bool     render_main = false;
	// iterate thru notifications and render them / erease them
	for (auto it = m_pop_notifications.begin(); it != m_pop_notifications.end();) {
		if ((*it)->get_finished()) {
			delete (*it);
			it = m_pop_notifications.erase(it);
		} else {
			PopNotification::RenderResult res = (*it)->render(canvas, last_x);
			if (res != PopNotification::RenderResult::Finished) {
				last_x = (*it)->get_top() + GAP_WIDTH;
				current_height = std::max(current_height, (*it)->get_current_top());
				render_main = true;
			}
			if (res == PopNotification::RenderResult::Moving || res == PopNotification::RenderResult::ClosePending || res == PopNotification::RenderResult::Finished)
				request_next_frame = true;
			++it;
		}
	}
	if (request_next_frame)
		canvas.request_extra_frame();
	//if (render_main)
	//	this->render_main_window(canvas, current_height);
}

void NotificationManager::render_main_window(GLCanvas3D& canvas, float height)
{
	Size             cnv_size = canvas.get_canvas_size();
	ImGuiWrapper&    imgui = *wxGetApp().imgui();
	bool             shown = true;
	std::string      name = "Notifications";
	imgui.set_next_window_pos(1.0f * (float)cnv_size.get_width(), 1.0f * (float)cnv_size.get_height(), ImGuiCond_Always, 1.0f, 1.0f);
	imgui.set_next_window_size(200, height + 30, ImGuiCond_Always);
	if (imgui.begin(name, &shown, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
		if (shown) {
		} else {
			// close all
			for(PopNotification* notification : m_pop_notifications)
			{
				notification->close();
			}
			canvas.set_as_dirty();
		}
	}
	imgui.end();
}

bool NotificationManager::find_older(NotificationType type)
{
	if (type == NotificationType::CustomNotification)
		return false;
	for (auto it = m_pop_notifications.begin(); it != m_pop_notifications.end(); ++it)
	{
		if((*it)->get_type() == type && !(*it)->get_finished())
		{
			if (it != m_pop_notifications.end() - 1)
				std::rotate(it, it + 1, m_pop_notifications.end());
			return true;
		}
	}
	return false;
}
void NotificationManager::print_to_console() const 
{
	/*
	std::cout << "---Notifications---" << std::endl;
	for (const Notification &notification :m_notification_container) {
		std::cout << "Notification " << ": " << notification.data.text << std::endl;
	}
	*/
}

}//namespace GUI
}//namespace Slic3r