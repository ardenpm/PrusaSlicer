#include "NotificationManager.hpp"

#include "GUI_App.hpp"
#include "GLCanvas3D.hpp"
#include "ImGuiWrapper.hpp"

#include <wx/glcanvas.h>
#include <iostream>


#define NOTIFICATION_MAX_MOVE 3.0f

namespace Slic3r {
namespace GUI {

//------PopNotification--------
NotificationManager::PopNotification::PopNotification(const NotificationData &n, const int id) :
	  m_data          (n)
	, m_id            (id)    
	, m_creation_time (wxGetLocalTime())
{
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

	imgui.set_next_window_pos(1.0f * (float)cnv_size.get_width(), 1.0f * (float)cnv_size.get_height() - m_current_x, ImGuiCond_Always, 1.0f, 0.0f);
	//set_next_window_size should be calculated with respect to size of all notifications and text
	imgui.set_next_window_size(200, 60, ImGuiCond_Always);

	//name of window - probably indentifies window and is shown so i add whitespaces according to id
	for (size_t i = 0; i < m_id; i++)
		name += " ";
	if (imgui.begin(name, &shown, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
		if (shown) {
			ImVec2 win_size = ImGui::GetWindowSize();
			m_window_height = win_size.y;
			m_window_width = win_size.x;
			
			//FIXME: dont forget to us this for texts
			//boost::format(_utf8(L(
			
			//way to set position of next element in window
			//ImGui::SetCursorPosX(50);

			//FIXME coloring of elements based on level
			ImGuiCol_ level_color_tag;
			switch (m_data.level) {
			    case NotificationLevel::RegularNotification: level_color_tag = ImGuiCol_WindowBg; break;
			    case NotificationLevel::ErrorNotification: level_color_tag = ImGuiCol_Header;  break;
			    case NotificationLevel::ImportantNotification: level_color_tag = ImGuiCol_Button; break;
			}
			//push&pop style color set color (first will heave color of second). pop after push or you get crash!  
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(level_color_tag));
			const ImVec4& backcolor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
			ImGui::PushStyleColor(ImGuiCol_Button, backcolor); 
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, backcolor);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, backcolor);

			//just text
			//imgui.text(m_data.text.c_str());

			//button - if part if treggered
			if (imgui.button(m_data.text.c_str(), 170, 40))
				m_close_pending = true;

			ImGui::PopStyleColor();
 			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			
		} else {
			// the user clicked on the [X] button ( ImGuiWindowFlags_NoTitleBar means theres no [X] button)
			m_close_pending = true;
			canvas.set_as_dirty();
		}
	}
	imgui.end();
	return ret_val;
}
void NotificationManager::PopNotification::update()
{
	m_creation_time = wxGetLocalTime();
}
//------NotificationManager--------
NotificationManager::NotificationManager()
{
}
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
		push_notification( *it, canvas);
}
void NotificationManager::push_notification(const std::string& text, GLCanvas3D& canvas)
{
	push_notification({ NotificationType::CustomNotification, NotificationLevel::RegularNotification, text, 10  }, canvas );
}
void NotificationManager::push_notification(const NotificationData &notification_data,  GLCanvas3D& canvas)
{
	if (!this->find_older(notification_data.type))
		m_pop_notifications.emplace_back(new PopNotification(notification_data, m_next_id++));
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
				last_x = (*it)->get_top();
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
	if (render_main)
		this->render_main_window(canvas, current_height);
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