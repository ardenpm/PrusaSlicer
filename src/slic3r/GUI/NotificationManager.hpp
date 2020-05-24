#ifndef slic3r_GUI_NotificationManager_hpp_
#define slic3r_GUI_NotificationManager_hpp_

#include "Event.hpp"

#include <string>
#include <vector>
#include <deque>

namespace Slic3r {
namespace GUI {

using EjectDriveNotificationClickedEvent = SimpleEvent;
wxDECLARE_EVENT(EVT_EJECT_DRIVE_NOTIFICAION_CLICKED, EjectDriveNotificationClickedEvent);

class GLCanvas3D;

enum class NotificationType
{
	CustomNotification,
	SlicingComplete,
	//DeviceEjected,
	ExportToRemovableFinished
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
		const std::string   text2 = std::string();
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
			Moving
		};
		PopNotification(const NotificationData &n, const int id, wxEvtHandler* evt_handler);
		//~PopNotificiation(){}
		RenderResult render(GLCanvas3D& canvas, const float& initial_x);
		// close will dissapear notification on next render
		void close() { m_close_pending = true; }
		// data from newer notification of same type
		void update();
		bool get_finished() const { return m_finished; }
		// returns top after movement
		float get_top() const { return m_target_x; }
		//returns top in actual frame
		float get_current_top() const { return m_current_x; }
		NotificationType get_type() const { return m_data.type; }
		
	private:
		void on_text_click();
		const NotificationData m_data;

		int           m_id;
		long          m_creation_time;
		bool          m_finished      { false }; // true - does not render, marked to delete
		bool          m_close_pending { false }; // will go to m_finished next render
		float         m_window_height { 0.0f };  
		float         m_window_width  { 0.0f };
		float         m_current_x     { 0.0f };  // x coord of top of window
		float         m_target_x      { 0.0f };  // x coord where top of window is moving to
		float         m_move_step     { 0.0f };  // movement in one render, calculated in first render
		wxEvtHandler* m_evt_handler;
	};


	NotificationManager(wxEvtHandler* evt_handler);
	~NotificationManager();

	
	// only type means one of basic_notification (see below)
	void push_notification(const NotificationType type, GLCanvas3D& canvas);
	// only text means Undefined type
	void push_notification(const std::string& text, GLCanvas3D& canvas);
	void push_notification(const std::string& text, NotificationLevel level, GLCanvas3D& canvas);
	// renders notifications in queue and deletes expired ones
	void render_notifications(GLCanvas3D& canvas);
	//pushes notification into the queue of notifications that are rendered
	//can be used to create custom notification
	void push_notification_data(const NotificationData& notification_data, GLCanvas3D& canvas);
private:
	void render_main_window(GLCanvas3D& canvas, float height);
	//finds older notification of same type and moves it to the end of queue. returns true if found
	bool find_older(NotificationType type);
	void print_to_console() const;

	wxEvtHandler* m_evt_handler;

	std::deque<PopNotification*> m_pop_notifications;
	int m_next_id{ 1 };

	//prepared notifications
	const std::vector<NotificationData> basic_notifications = {
		{NotificationType::SlicingComplete, NotificationLevel::RegularNotification, 5, "Slicing finished"/*, "clickable", "fisnisher" */},
		{NotificationType::ExportToRemovableFinished, NotificationLevel::ImportantNotification, 0, "Exporting finished.", "Eject drive." },
		//{NotificationType::DeviceEjected, NotificationLevel::RegularNotification, 10, "Removable device has been safely ejected"} // if we want changeble text (like here name of device), we need to do it as CustomNotification
		//{NotificationType::SlicingComplete, NotificationLevel::ImportantNotification, 10, Unmounting successful.The device% s(% s) can now be safely removed from the computer." }
		//			Slic3r::GUI::show_info(this->q, format_wxstr(_L("Unmounting successful. The device %s(%s) can now be safely removed from the computer."),

	};
};

}//namespace GUI
}//namespace Slic3r

#endif //slic3r_GUI_NotificationManager_hpp_