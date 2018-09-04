typedef struct _EventInfo
{
	std::string name;
	std::string sourceName; // the log file name
	int         sourcePos;  // the location of log file where parsed this event from

} EventInfo;

typedef std::queue <EventInfo> EventQueue;

typedef std::map <std::string, EventQueue> EventMap; // map from event name to event queue

typedef std::map <std::string, EventSubscriber> SubscriberMap; // map from the event name to subscriber

