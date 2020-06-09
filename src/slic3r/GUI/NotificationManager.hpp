#ifndef slic3r_GUI_NotificationManager_hpp_
#define slic3r_GUI_NotificationManager_hpp_

#include "Event.hpp"

#include <string>
#include <vector>
#include <deque>
#include <unordered_set>

namespace Slic3r {
namespace GUI {

using EjectDriveNotificationClickedEvent = SimpleEvent;
wxDECLARE_EVENT(EVT_EJECT_DRIVE_NOTIFICAION_CLICKED, EjectDriveNotificationClickedEvent);
using ExportGcodeNotificationClickedEvent = SimpleEvent;
wxDECLARE_EVENT(EVT_EXPORT_GCODE_NOTIFICAION_CLICKED, ExportGcodeNotificationClickedEvent);

class GLCanvas3D;
class ImGuiWrapper;

enum class NotificationType
{
	CustomNotification,
	SlicingComplete,
	SlicingNotPossible,
	ExportToRemovableFinished,
	Mouse3dDisconnected,
	Mouse3dConnected,
	NewPresetsAviable,
	NewAppAviable,
	PresetUpdateAviable,
	LoadingFailed,
	ValidateError

};
class NotificationManager
{
public:
	enum class NotificationLevel
	{
		RegularNotification,
		ErrorNotification,
		ImportantNotification
	};
	// duration 0 means not disapearing
	struct NotificationData {
		NotificationType    type;
		NotificationLevel   level;
		const int           duration;
		const std::string   text1;
		const std::string   hypertext = std::string();
		const std::string   text2     = std::string();
	};

	//Pop notification - shows only once to user.
	class PopNotification
	{
	public:
		enum class RenderResult
		{
			Finished,
			ClosePending,
			Static,
			Countdown,
			Hovered
		};
		 PopNotification(const NotificationData &n, const int id, wxEvtHandler* evt_handler);
		~PopNotification();
		RenderResult           render(GLCanvas3D& canvas, const float& initial_x, bool gray);
		// close will dissapear notification on next render
		void                   close() { m_close_pending = true; }
		// data from newer notification of same type
		void                   update(const NotificationData& n);
		bool                   get_finished() const { return m_finished; }
		// returns top after movement
		float                  get_top() const { return m_top_x; }
		//returns top in actual frame
		float                  get_current_top() const { return m_top_x; }
		const NotificationType get_type() const { return m_data.type; }
		const NotificationData get_data() const { return m_data;  }
		void                   substract_remaining_time() { m_remaining_time--; }
	protected:
		virtual void set_next_window_size(ImGuiWrapper& imgui);
		virtual void render_text(ImGuiWrapper& imgui,
			                     const float win_size_x, const float win_size_y,
			                     const float win_pos_x , const float win_pos_y);
		void         render_close_button(ImGuiWrapper& imgui,
			                             const float win_size_x, const float win_size_y,
			                             const float win_pos_x , const float win_pos_y);
		void         render_countdown(ImGuiWrapper& imgui,
			                          const float win_size_x, const float win_size_y,
			                          const float win_pos_x , const float win_pos_y);
		void         render_hypertext(ImGuiWrapper& imgui,
			                          const float text_x, const float text_y,
		                              const std::string text);
		void         on_text_click();

		const NotificationData m_data;

		int           m_id;
		std::string   m_text1;
		std::string   m_hypertext;
		std::string   m_text2;
		long          m_remaining_time;
		bool          m_counting_down;
		bool          m_finished        { false }; // true - does not render, marked to delete
		bool          m_close_pending   { false }; // will go to m_finished next render
		float         m_window_height   { 55.0f };  
		float         m_window_width    { 450.0f };
		float         m_top_x           { 0.0f };  // x coord where top of window is moving to
		int           m_lines_count     { 1 };
		wxEvtHandler* m_evt_handler;
	};

	class SlicingCompleteLargeNotification : public PopNotification
	{
	public:
		SlicingCompleteLargeNotification(const NotificationData& n, const int id, wxEvtHandler* evt_handler, bool largeds);
		void set_large(bool l);
		bool get_large() { return m_is_large; }
		void set_print_info(std::string info);
	protected:
		virtual void render_text(ImGuiWrapper& imgui,
			                     const float win_size_x, const float win_size_y,
			                     const float win_pos_x, const float win_pos_y) 
			                     override;

		bool        m_is_large;
		bool        m_has_print_info { false };
		std::string m_print_info { std::string() };
	};

	NotificationManager(wxEvtHandler* evt_handler);
	~NotificationManager();

	
	// only type means one of basic_notification (see below)
	void push_notification(const NotificationType type, GLCanvas3D& canvas, int timestamp = 0);
	// only text means Undefined type
	void push_notification(const std::string& text, GLCanvas3D& canvas, int timestamp = 0);
	void push_notification(const std::string& text, NotificationLevel level, GLCanvas3D& canvas, int timestamp = 0);
	// creates ValidateError notification with custom text
	void push_validate_error_notification(const std::string& text, GLCanvas3D& canvas);
	// creates special notification slicing complete
	// if large = true prints printing time and export button 
	void push_slicing_complete_notification(GLCanvas3D& canvas, int timestamp, bool large);
	void set_slicing_complete_print_time(std::string info);
	void set_slicing_complete_large(bool large);
	// renders notifications in queue and deletes expired ones
	void render_notifications(GLCanvas3D& canvas);
	
	void set_validate_error_gray() { m_validate_error_gray = true; }
private:
	//pushes notification into the queue of notifications that are rendered
	//can be used to create custom notification
	bool push_notification_data(const NotificationData& notification_data, GLCanvas3D& canvas, int timestamp);
	bool push_notification_data(NotificationManager::PopNotification* notification, GLCanvas3D& canvas, int timestamp);
	void render_main_window(GLCanvas3D& canvas, float height);
	//finds older notification of same type and moves it to the end of queue. returns true if found
	bool find_older(NotificationType type);
	void print_to_console() const;

	wxEvtHandler* m_evt_handler;

	std::deque<PopNotification*> m_pop_notifications;
	int m_next_id{ 1 };

	long m_last_time { 0 };
	bool m_validate_error_gray { false };

	std::unordered_set<int> m_used_timestamps;

	//prepared notifications
	const std::vector<NotificationData> basic_notifications = {
		//{NotificationType::SlicingComplete, NotificationLevel::RegularNotification, 10, "Slicing finished."/*, "clickable", "fisnisher" */},
		{NotificationType::SlicingNotPossible, NotificationLevel::RegularNotification, 10, "Slicing is not possible."},
		{NotificationType::ExportToRemovableFinished, NotificationLevel::ImportantNotification, 0, "Exporting finished.", "Eject drive." },
		{NotificationType::Mouse3dDisconnected, NotificationLevel::RegularNotification, 10, "3D Mouse disconnected." },
		{NotificationType::Mouse3dConnected, NotificationLevel::RegularNotification, 5, "3D Mouse connected." },
		{NotificationType::NewPresetsAviable, NotificationLevel::RegularNotification, 15, "New Presets are aviable.", "See here." },
		{NotificationType::NewAppAviable, NotificationLevel::RegularNotification, 15, "New vesion of PrusaSlicer is aviable.", "Download." },
		{NotificationType::PresetUpdateAviable, NotificationLevel::RegularNotification, 5, "Preset update is aviable.", "Update."},
		//{NotificationType::LoadingFailed, NotificationLevel::RegularNotification, 20, "Loading of model has Failed" },

		//{NotificationType::DeviceEjected, NotificationLevel::RegularNotification, 10, "Removable device has been safely ejected"} // if we want changeble text (like here name of device), we need to do it as CustomNotification
		//{NotificationType::SlicingComplete, NotificationLevel::ImportantNotification, 10, Unmounting successful.The device% s(% s) can now be safely removed from the computer." }
		//			Slic3r::GUI::show_info(this->q, format_wxstr(_L("Unmounting successful. The device %s(%s) can now be safely removed from the computer."),

	};
};

}//namespace GUI
}//namespace Slic3r

#endif //slic3r_GUI_NotificationManager_hpp_