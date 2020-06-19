#include "NotificationManager.hpp"

#include "GUI_App.hpp"
#include "Plater.hpp"
#include "GLCanvas3D.hpp"
#include "ImGuiWrapper.hpp"

#include "wxExtensions.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>
#include <wx/glcanvas.h>
#include <iostream>




#define NOTIFICATION_MAX_MOVE 3.0f

#define GAP_WIDTH 10.0f
#define SPACE_RIGHT_PANEL 10.0f

namespace Slic3r {
namespace GUI {

wxDEFINE_EVENT(EVT_EJECT_DRIVE_NOTIFICAION_CLICKED, EjectDriveNotificationClickedEvent);
wxDEFINE_EVENT(EVT_EXPORT_GCODE_NOTIFICAION_CLICKED, ExportGcodeNotificationClickedEvent);
wxDEFINE_EVENT(EVT_PRESET_UPDATE_AVIABLE_CLICKED, PresetUpdateAviableClickedEvent);

//ScalableBitmap bmp_icon;
//------PopNotification--------
NotificationManager::PopNotification::PopNotification(const NotificationData &n, const int id, wxEvtHandler* evt_handler) :
	  m_data           (n)
	, m_id             (id)    
	, m_remaining_time (n.duration)
	, m_counting_down  (n.duration != 0)
	, m_text1          (n.text1)
    , m_hypertext      (n.hypertext)
    , m_text2          (n.text2)
	, m_evt_handler    (evt_handler)
{
	count_lines();
}
NotificationManager::PopNotification::~PopNotification()
{
}
NotificationManager::PopNotification::RenderResult NotificationManager::PopNotification::render(GLCanvas3D& canvas, const float& initial_x)
{
	if (m_finished)
		return RenderResult::Finished;
	
	RenderResult    ret_val = m_counting_down ? RenderResult::Countdown : RenderResult::Static;
	Size            cnv_size = canvas.get_canvas_size();
	ImGuiWrapper&   imgui = *wxGetApp().imgui();
	bool            new_target = false;
	bool            shown = true;
	std::string     name;
	
	//background color
	if (m_is_gray)
	{
		ImVec4 backcolor(0.7f, 0.7f, 0.7f, 0.5f);
	    ImGui::PushStyleColor(ImGuiCol_WindowBg, backcolor);
	}
	//top y of window
	m_top_x = initial_x + m_window_height;
	//top right position
	ImVec2 win_pos(1.0f * (float)cnv_size.get_width() - SPACE_RIGHT_PANEL, 1.0f * (float)cnv_size.get_height() - m_top_x);
	imgui.set_next_window_pos(win_pos.x, win_pos.y, ImGuiCond_Always, 1.0f, 0.0f);

	set_next_window_size(imgui);

	ImVec2 mouse_pos = ImGui::GetMousePos();
	//BOOST_LOG_TRIVIAL(error) << mouse_pos.x << "," << mouse_pos.y;

	//find if hovered
	if (mouse_pos.x < win_pos.x && mouse_pos.x > win_pos.x - m_window_width && mouse_pos.y > win_pos.y&& mouse_pos.y < win_pos.y + m_window_height)
	{
		//BOOST_LOG_TRIVIAL(error) << "mouse";
		ImGui::SetNextWindowFocus();
		ret_val = RenderResult::Hovered;
	}

	if (m_counting_down && m_remaining_time < 0)
		m_close_pending = true;

	if (m_close_pending) {
		// request of extra frame will be done in caller function by ret val ClosePending
		m_finished = true;
		return RenderResult::ClosePending;
	}

	//name of window - probably indentifies window and is shown so last_end add whitespaces according to id
	for (size_t i = 0; i < m_id; i++)
		name += " ";
	if (imgui.begin(name, &shown, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar )) {
		if (shown) {
			if (m_is_gray)
				ImGui::PopStyleColor();
			ImVec2 win_size = ImGui::GetWindowSize();
			
			
			//FIXME: dont forget to us this for texts
			//boost::format(_utf8(L(
			
			/*
			//countdown numbers
			ImGui::SetCursorPosX(15);
			ImGui::SetCursorPosY(15);
			imgui.text(std::to_string(m_remaining_time).c_str());
			*/
			if(m_counting_down)
				render_countdown(imgui, win_size.x, win_size.y, win_pos.x, win_pos.y);
			render_text(imgui, win_size.x, win_size.y, win_pos.x, win_pos.y);
			render_close_button(imgui, win_size.x, win_size.y, win_pos.x, win_pos.y);
			
		} else {
			// the user clicked on the [X] button ( ImGuiWindowFlags_NoTitleBar means theres no [X] button)
			m_close_pending = true;
			canvas.set_as_dirty();
		}
	}
	imgui.end();
	return ret_val;
}
void NotificationManager::PopNotification::count_lines()
{
	std::string text = m_text1 + " " + m_hypertext;
	int last_end = 0;
	m_lines_count = 0;
	m_endlines.clear();
	while (last_end < text.length() - 1)
	{
		int next_hard_end = text.find_first_of('\n', last_end);
		if (next_hard_end > 0 && ImGui::CalcTextSize(text.substr(last_end, next_hard_end - last_end).c_str()).x < m_window_width - m_window_width_offset) {
			//next line is ended by '/n'
			m_endlines.push_back(next_hard_end);
			last_end = next_hard_end + 1;
		}
		else {
			// find next suitable endline
			if (ImGui::CalcTextSize(text.substr(last_end).c_str()).x >= m_window_width - m_window_width_offset) {
				// more than one line till end
				int next_space = text.find_first_of(' ', last_end);
				if (next_space > 0) {
					int next_space_candidate = text.find_first_of(' ', next_space + 1);
					while (next_space_candidate > 0 && ImGui::CalcTextSize(text.substr(last_end, next_space_candidate - last_end).c_str()).x < m_window_width - m_window_width_offset) {
						next_space = next_space_candidate;
						next_space_candidate = text.find_first_of(' ', next_space + 1);
					}
					m_endlines.push_back(next_space);
					last_end = next_space + 1;
				}
			}
			else {
				m_endlines.push_back(text.length());
				last_end = text.length();
			}

		}
		m_lines_count++;
	}
}
void NotificationManager::PopNotification::set_next_window_size(ImGuiWrapper& imgui)
{
	//m_lines_count = text1_size.x / (m_window_width - m_window_width_offset) + 1;
	//m_lines_count += m_hard_newlines;
	if (m_multiline) {
		m_window_height = m_window_height_base + m_lines_count * 19;//11;
	}
	imgui.set_next_window_size(m_window_width, m_window_height, ImGuiCond_Always);
}

void NotificationManager::PopNotification::render_text(ImGuiWrapper& imgui, const float win_size_x, const float win_size_y, const float win_pos_x, const float win_pos_y)
{
	ImVec2 win_size(win_size_x, win_size_y);
	ImVec2 win_pos(win_pos_x, win_pos_y);
	ImVec2 text1_size = ImGui::CalcTextSize(m_text1.c_str());
	float x_offset = 20;
	std::string fulltext = m_text1 + m_hypertext + m_text2;
	ImVec2 text_size = ImGui::CalcTextSize(fulltext.c_str());
	float line_height = ImGui::CalcTextSize("abcdefghijklmnopqrstuvwxyz0123456789").y;
	// text posistions are calculated by lines count
	// large texts has "more" button or are displayed whole
	// smaller texts are divided as one liners and two liners
	if (m_lines_count > 2) {
		if (m_multiline) {
			int last_end = 0;
			float starting_y = 10;//text_size.y/2;
			float shift_y = line_height;
			for (size_t i = 0; i < m_lines_count; i++) {
			    std::string line = m_text1.substr(last_end , m_endlines[i] - last_end);
				last_end = m_endlines[i] + 1;
				ImGui::SetCursorPosX(x_offset);
				ImGui::SetCursorPosY(starting_y + i * shift_y);
				imgui.text(line.c_str());
			}
			//hyperlink text
			if (!m_hypertext.empty())
			{
				ImVec2 prev_size = ImGui::CalcTextSize(m_text1.c_str());
				render_hypertext(imgui, x_offset + ImGui::CalcTextSize(m_text1.substr(m_endlines[m_lines_count - 2] + 1, m_endlines[m_lines_count - 1] - m_endlines[m_lines_count - 2] - 1).c_str()).x, starting_y + (m_lines_count - 1) * shift_y, m_hypertext);
			}
		} else {
			// line1
			ImGui::SetCursorPosX(x_offset);
			ImGui::SetCursorPosY(win_size.y / 2 - win_size.y / 6 - line_height / 2);
			imgui.text(m_text1.substr(0, m_endlines[0]).c_str());
			// line2
			std::string line = m_text1.substr(m_endlines[0] + 1, m_endlines[1] - m_endlines[0] - 1);
			if (ImGui::CalcTextSize(line.c_str()).x > m_window_width - m_window_width_offset - ImGui::CalcTextSize("..More").x)
				line = line.substr(0, line.length() - 6);
			line += "..";
			ImGui::SetCursorPosX(x_offset);
			ImGui::SetCursorPosY(win_size.y / 2 + win_size.y / 6 - line_height / 2);
			imgui.text(line.c_str());
			// "More" hypertext
			render_hypertext(imgui, x_offset + ImGui::CalcTextSize(line.c_str()).x, win_size.y / 2 + win_size.y / 6 - line_height / 2, "More", true);
		}
	} else {
		//text 1
		float cursor_y = win_size.y / 2 - text_size.y / 2;
		float cursor_x = x_offset;
		if(m_lines_count > 1) {
			// line1
			ImGui::SetCursorPosX(x_offset);
			ImGui::SetCursorPosY(win_size.y / 2 - win_size.y / 6 - line_height / 2);
			imgui.text(m_text1.substr(0, m_endlines[0]).c_str());
			// line2
			std::string line = m_text1.substr(m_endlines[0] + 1);
			cursor_y = win_size.y / 2 + win_size.y / 6 - line_height / 2;
			ImGui::SetCursorPosX(x_offset);
			ImGui::SetCursorPosY(cursor_y);
			imgui.text(line.c_str());
			cursor_x = x_offset + ImGui::CalcTextSize(line.c_str()).x;
		} else {
			ImGui::SetCursorPosX(/*win_size.x / 2 - text_size.x / 2 - */x_offset);
			ImGui::SetCursorPosY(cursor_y);
			imgui.text(m_text1.c_str());
			cursor_x = x_offset + ImGui::CalcTextSize(m_text1.c_str()).x;
		}
		//hyperlink text
		if (!m_hypertext.empty())
		{
			ImVec2 prev_size = ImGui::CalcTextSize(m_text1.c_str());
			render_hypertext(imgui, cursor_x + 4, cursor_y, m_hypertext);
		}

		//notification text 2
		//text 2 is suposed to be after the hyperlink - currently it is not used
		/*
		if (!m_text2.empty())
		{
			ImVec2 part_size = ImGui::CalcTextSize(m_hypertext.c_str());
			ImGui::SetCursorPosX(win_size.x / 2 + text_size.x / 2 - part_size.x + 8 - x_offset);
			ImGui::SetCursorPosY(cursor_y);
			imgui.text(m_text2.c_str());
		}
		*/
	}


	
	
}

void NotificationManager::PopNotification::render_hypertext(ImGuiWrapper& imgui, const float text_x, const float text_y, const std::string text, bool more)
{
	//invisible button
	ImVec2 part_size = ImGui::CalcTextSize(text.c_str());
	ImGui::SetCursorPosX(text_x -4);
	ImGui::SetCursorPosY(text_y -5);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.0f, .0f, .0f, .0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.0f, .0f, .0f, .0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.0f, .0f, .0f, .0f));
	if (imgui.button("", part_size.x + 6, part_size.y + 10))
	{
		if (more)
		{
			m_multiline = true;
			set_next_window_size(imgui);
		}
		else {
			on_text_click();
			m_close_pending = true;
		}
	}
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();

	//hover color
	ImVec4 orange_color = ImGui::GetStyleColorVec4(ImGuiCol_Button);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
		orange_color.y += 0.2f;

	//text
	ImGui::PushStyleColor(ImGuiCol_Text, orange_color);
	ImGui::SetCursorPosX(text_x);
	ImGui::SetCursorPosY(text_y);
	imgui.text(text.c_str());
	ImGui::PopStyleColor();

	//underline
	ImVec2 lineEnd = ImGui::GetItemRectMax();
	lineEnd.y -= 2;
	ImVec2 lineStart = lineEnd;
	lineStart.x = ImGui::GetItemRectMin().x;
	ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, IM_COL32((int)(orange_color.x * 255), (int)(orange_color.y * 255), (int)(orange_color.z * 255), (int)(orange_color.w * 255)));

}

void NotificationManager::PopNotification::render_close_button(ImGuiWrapper& imgui, const float win_size_x, const float win_size_y, const float win_pos_x, const float win_pos_y)
{
	ImVec2 win_size(win_size_x, win_size_y);
	ImVec2 win_pos(win_pos_x, win_pos_y);
	ImVec4 orange_color = ImGui::GetStyleColorVec4(ImGuiCol_Button);
	orange_color.w = 0.8f;
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.0f, .0f, .0f, .0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.0f, .0f, .0f, .0f));
	//ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.0f, .0f, .0f, .0f));
	//ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));

	//ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.75f, .75f, .75f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0, .75f, .75f, 1.f));

	ImGui::SetCursorPosX(win_size.x - 45);
	ImGui::SetCursorPosY(win_size.y / 2 - 15);
	//button - if part if treggered
	std::string button_text;
	button_text = ImGui::CloseIconMarker;
	if (ImGui::IsMouseHoveringRect(ImVec2(win_pos.x - 45, win_pos.y + win_size.y / 2 - 15),
		ImVec2(win_pos.x - 15, win_pos.y + win_size.y / 2 + 15),
		true))
	{
		button_text = ImGui::CloseIconHoverMarker;
	}
	if (imgui.button(button_text.c_str(), 30, 30))
	{
		m_close_pending = true;
	}

	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
}
void NotificationManager::PopNotification::render_countdown(ImGuiWrapper& imgui, const float win_size_x, const float win_size_y, const float win_pos_x, const float win_pos_y)
{
	ImVec2 win_size(win_size_x, win_size_y);
	ImVec2 win_pos(win_pos_x, win_pos_y);

	//countdown dots
	std::string dot_text;
	dot_text = m_remaining_time <= (float)m_data.duration / 4 * 3 ? ImGui::TimerDotEmptyMarker : ImGui::TimerDotMarker;
	ImGui::SetCursorPosX(win_size.x - 21);
	//ImGui::SetCursorPosY(win_size.y / 2 - 24);
	ImGui::SetCursorPosY(0);
	imgui.text(dot_text.c_str());

	dot_text = m_remaining_time < m_data.duration / 2 ? ImGui::TimerDotEmptyMarker : ImGui::TimerDotMarker;
	ImGui::SetCursorPosX(win_size.x - 21);
	//ImGui::SetCursorPosY(win_size.y / 2 - 9);
	ImGui::SetCursorPosY(win_size.y / 2 - 11);
	imgui.text(dot_text.c_str());

	dot_text = m_remaining_time <= m_data.duration / 4 ? ImGui::TimerDotEmptyMarker : ImGui::TimerDotMarker;
	ImGui::SetCursorPosX(win_size.x - 21);
	//ImGui::SetCursorPosY(win_size.y / 2 + 6);
	ImGui::SetCursorPosY(win_size.y - 22);
	imgui.text(dot_text.c_str());

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
	case NotificationType::SlicingComplete :
		//wxGetApp().plater()->export_gcode(false);
		assert(m_evt_handler != nullptr);
		if (m_evt_handler != nullptr)
			wxPostEvent(m_evt_handler, ExportGcodeNotificationClickedEvent(EVT_EXPORT_GCODE_NOTIFICAION_CLICKED));
	case NotificationType::PresetUpdateAviable :
		//wxGetApp().plater()->export_gcode(false);
		assert(m_evt_handler != nullptr);
		if (m_evt_handler != nullptr)
			wxPostEvent(m_evt_handler, PresetUpdateAviableClickedEvent(EVT_PRESET_UPDATE_AVIABLE_CLICKED));
	default:
		break;
	}
}
void NotificationManager::PopNotification::update(const NotificationData& n)
{
	m_text1          = n.text1;
	m_hypertext      = n.hypertext;
    m_text2          = n.text2;
	count_lines();
}
//SlicingCompleteLargeNotification
NotificationManager::SlicingCompleteLargeNotification::SlicingCompleteLargeNotification(const NotificationData& n, const int id, wxEvtHandler* evt_handler, bool large) :
	  NotificationManager::PopNotification(n, id, evt_handler)
	, m_is_large (large)
{}
void NotificationManager::SlicingCompleteLargeNotification::render_text(ImGuiWrapper& imgui, const float win_size_x, const float win_size_y, const float win_pos_x, const float win_pos_y)
{
	if (!m_is_large)
		PopNotification::render_text(imgui, win_size_x, win_size_y, win_pos_x, win_pos_y);
	else {
		ImVec2 win_size(win_size_x, win_size_y);
		ImVec2 win_pos(win_pos_x, win_pos_y);

		ImVec2 text1_size = ImGui::CalcTextSize(m_text1.c_str());
		float x_offset = 20;
		std::string fulltext = m_text1 + m_hypertext + m_text2;
		ImVec2 text_size = ImGui::CalcTextSize(fulltext.c_str());
		float cursor_y = win_size.y / 2 - text_size.y / 2;
		if (m_has_print_info) {
			x_offset = 20;
			cursor_y = win_size.y / 2 - win_size.y / 6 - text_size.y / 2;
			int half = m_text1.find_first_of(' ', m_text1.length() / 2 - 1);
			ImVec2 info_size = ImGui::CalcTextSize(m_print_info.c_str());
			ImGui::SetCursorPosX(/*win_size.x / 2 - info_size.x / 2 - */x_offset);
			ImGui::SetCursorPosY(cursor_y);
			imgui.text(m_print_info.c_str());
			cursor_y = win_size.y / 2 + win_size.y / 6 - text_size.y / 2;
		}
		ImGui::SetCursorPosX(/*win_size.x / 2 - text_size.x / 2 - */x_offset);
		ImGui::SetCursorPosY(cursor_y);
		imgui.text(m_text1.c_str());

		ImVec2 prev_size = ImGui::CalcTextSize(m_text1.c_str());
		render_hypertext(imgui, /*win_size.x / 2 - text_size.x / 2 + prev_size.x + 4 - */ x_offset + text1_size.x + 4, cursor_y, m_hypertext);
		
	}
}
void NotificationManager::SlicingCompleteLargeNotification::set_print_info(std::string info)
{
	m_print_info = info;
	m_has_print_info = true;
	if(m_is_large)
		m_lines_count = 2;
}
void NotificationManager::SlicingCompleteLargeNotification::set_large(bool l)
{
	m_is_large = l;
	m_counting_down = !l;
	m_hypertext = l ? "Export G-Code." : std::string();
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
void NotificationManager::push_notification(const NotificationType type, GLCanvas3D& canvas, int timestamp)
{
	auto it = std::find_if(basic_notifications.begin(), basic_notifications.end(),
		boost::bind(&NotificationData::type, _1) == type);	
	if (it != basic_notifications.end())
		push_notification_data( *it, canvas, timestamp);
}
void NotificationManager::push_notification(const std::string& text, GLCanvas3D& canvas, int timestamp)
{
	push_notification_data({ NotificationType::CustomNotification, NotificationLevel::RegularNotification, 10, text }, canvas, timestamp );
}
void NotificationManager::push_notification(const std::string& text, NotificationManager::NotificationLevel level, GLCanvas3D& canvas, int timestamp)
{
	switch (level)
	{
	case Slic3r::GUI::NotificationManager::NotificationLevel::RegularNotification:
		push_notification_data({ NotificationType::CustomNotification, level, 10, text }, canvas, timestamp);
		break;
	case Slic3r::GUI::NotificationManager::NotificationLevel::ErrorNotification:
		push_notification_data({ NotificationType::CustomNotification, level, 0, text }, canvas, timestamp);

		break;
	case Slic3r::GUI::NotificationManager::NotificationLevel::ImportantNotification:
		push_notification_data({ NotificationType::CustomNotification, level, 0, text }, canvas, timestamp);
		break;
	default:
		break;
	}
}
void NotificationManager::push_error_notification(const std::string& text, GLCanvas3D& canvas)
{
	set_error_gray(false);
	push_notification_data({ NotificationType::SlicingError, NotificationLevel::ErrorNotification, 0, "ERROR:\n" + text }, canvas, 0);
}
void NotificationManager::push_warning_notification(const std::string& text, GLCanvas3D& canvas)
{
	//set_error_gray(false);
	push_notification_data({ NotificationType::SlicingWarning, NotificationLevel::WarningNotification, 0, "WARNING:\n" + text }, canvas, 0);
}

void NotificationManager::set_error_gray(bool g)
{
	for (PopNotification* notification : m_pop_notifications) {
		if (notification->get_type() == NotificationType::SlicingError || notification->get_type() == NotificationType::SlicingWarning) {
			notification->set_gray(g);
		}
	}
}
void NotificationManager::clear_error()
{
	for (PopNotification* notification : m_pop_notifications) {
		if (notification->get_type() == NotificationType::SlicingError || notification->get_type() == NotificationType::SlicingWarning) {
			notification->close();
		}
	}
}
void NotificationManager::push_slicing_complete_notification(GLCanvas3D& canvas, int timestamp, bool large)
{
	std::string hypertext;
	int time = 10;
	if(large)
	{
		hypertext = "Export G-Code.";
		time = 0;
	}
	NotificationData data{ NotificationType::SlicingComplete, NotificationLevel::RegularNotification, time, "Slicing finished.", hypertext };

	NotificationManager::SlicingCompleteLargeNotification* notification = new NotificationManager::SlicingCompleteLargeNotification(data, m_next_id++, m_evt_handler, large);
	if (push_notification_data(notification, canvas, timestamp)){
	} else {
		delete notification;
	}	
}
void NotificationManager::set_slicing_complete_print_time(std::string info)
{
	for (PopNotification* notification : m_pop_notifications) {
		if (notification->get_type() == NotificationType::SlicingComplete) {
			dynamic_cast<SlicingCompleteLargeNotification*>(notification)->set_print_info(info);
			break;
		}
	}
}
void NotificationManager::set_slicing_complete_large(bool large)
{
	for (PopNotification* notification : m_pop_notifications) {
		if (notification->get_type() == NotificationType::SlicingComplete) {
			dynamic_cast<SlicingCompleteLargeNotification*>(notification)->set_large(large);
			break;
		}
	}
}

bool NotificationManager::push_notification_data(const NotificationData &notification_data,  GLCanvas3D& canvas, int timestamp)
{
	PopNotification* n = new PopNotification(notification_data, m_next_id++, m_evt_handler);
	bool r = push_notification_data(n, canvas, timestamp);
	if (!r)
		delete n;
	return r;
	/*
	if(timestamp != 0)
		if (m_used_timestamps.find(timestamp) == m_used_timestamps.end())
			m_used_timestamps.insert(timestamp);
		else
			return false;

	if (!this->find_older(notification_data.type)) {
		m_pop_notifications.emplace_back(new PopNotification(notification_data, m_next_id++, m_evt_handler));
		canvas.request_extra_frame();
		return true;
	} else {
		m_pop_notifications.back()->update(notification_data);
		canvas.request_extra_frame();
		return false;
	}
	*/
}
bool NotificationManager::push_notification_data(NotificationManager::PopNotification* notification, GLCanvas3D& canvas, int timestamp)
{
	if (timestamp != 0)
		if (m_used_timestamps.find(timestamp) == m_used_timestamps.end())
			m_used_timestamps.insert(timestamp);
		else
			return false;

	if (!this->find_older(notification->get_type())) {
		m_pop_notifications.emplace_back(notification);
		canvas.request_extra_frame();
		return true;
	}
	else {
		m_pop_notifications.back()->update(notification->get_data());
		canvas.request_extra_frame();
		return false;
	}
}
void NotificationManager::render_notifications(GLCanvas3D& canvas)
{
	float    last_x = 0.0f;
	float    current_height = 0.0f;
	bool     request_next_frame = false;
	bool     render_main = false;
	bool     hovered = false;

	
	
	//BOOST_LOG_TRIVIAL(error) << "render";
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
			if (res == PopNotification::RenderResult::Countdown || res == PopNotification::RenderResult::ClosePending || res == PopNotification::RenderResult::Finished)
				request_next_frame = true;
			if (res == PopNotification::RenderResult::Hovered)
				hovered = true;
			++it;
		}
	}

	//detect if main window is active
	//if it is, actualizate timers and request frame if needed
	wxWindow* p = dynamic_cast<wxWindow*> (wxGetApp().plater());
	while (p->GetParent())
		p = p->GetParent();
	wxTopLevelWindow* top_level_wnd = dynamic_cast<wxTopLevelWindow*>(p);
	if (!top_level_wnd->IsActive())
		return;

	if (!hovered && m_last_time < wxGetLocalTime())
	{
		if (wxGetLocalTime() - m_last_time == 1)
		{
			for each (auto notification in m_pop_notifications)
			{
				notification->substract_remaining_time();
			}
		}
		m_last_time = wxGetLocalTime();
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