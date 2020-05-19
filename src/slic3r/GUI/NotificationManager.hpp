#ifndef slic3r_GUI_NotificationManager_hpp_
#define slic3r_GUI_NotificationManager_hpp_

#include <string>
#include <vector>
#include <deque>

namespace Slic3r {
namespace GUI {


class GLCanvas3D;

enum class NotificationType
{
	CustomNotification,
	SlicingComplete,
	DeviceEjected
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
		const std::string   text;
		const int           duration;
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
		PopNotification(const NotificationData &n, const int id);
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
		const NotificationData m_data;

		int   m_id;
		long  m_creation_time;
		bool  m_finished      { false }; // true - does not render, marked to delete
		bool  m_close_pending { false }; // will go to m_finished next render
		float m_window_height { 0.0f };  
		float m_window_width  { 0.0f };
		float m_current_x     { 0.0f };  // x coord of top of window
		float m_target_x      { 0.0f };  // x coord where top of window is moving to
		float m_move_step     { 0.0f };  // movement in one render, calculated in first render
	};


	NotificationManager();
	~NotificationManager();

	
	// only type means one of basic_notification (see below)
	void push_notification(const NotificationType type, GLCanvas3D& canvas);
	// only text means Undefined type
	void push_notification(const std::string& text, GLCanvas3D& canvas);
	// renders notifications in queue and deletes expired ones
	void render_notifications(GLCanvas3D& canvas);
	//pushes notification into the queue of notifications that are rendered
	//can be used to create custom notification
	void push_notification(const NotificationData& notification_data, GLCanvas3D& canvas);
private:
	void render_main_window(GLCanvas3D& canvas, float height);
	//finds older notification of same type and moves it to the end of queue. returns true if found
	bool find_older(NotificationType type);
	void print_to_console() const;

	std::deque<PopNotification*> m_pop_notifications;
	int m_next_id{ 1 };

	//prepared notifications
	const std::vector<NotificationData> basic_notifications = {
		{NotificationType::SlicingComplete, NotificationLevel::ImportantNotification, "Slicing finished", 0},
		{NotificationType::DeviceEjected, NotificationLevel::RegularNotification, "Removable device has been safely ejected", 10} // if we want changeble text (like here name of device), we need to do it as CustomNotification
	};
};

}//namespace GUI
}//namespace Slic3r

#endif //slic3r_GUI_NotificationManager_hpp_