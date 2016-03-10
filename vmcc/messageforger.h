#ifndef SINGHALCOMMONPARSER_H
#define SINGHALCOMMONPARSER_H

#include <string>
#include <string.h>
#include "jsonxx.h"

#define KEY_TYPE                    "type"
#define KEY_SENDER                  "sender"
#define KEY_RECEIVER                "receiver"
#define KEY_SITES_COUNT             "sites-count"
#define KEY_INITIAL_TOKEN_HOLDER    "initial-token-holder"
#define KEY_REQUESTER_SN            "requester-sn"
#define KEY_FILE_NAME               "file-name"
#define KEY_FILE_CONTENT            "file-content"
#define KEY_TSN                     "tsn"
#define KEY_TSV                     "tsv"
#define KEY_SN                      "sn"
#define KEY_SV                      "sv"

using namespace std;

enum SiteState{
    // Request
    STATE_R,
    // Execute
    STATE_E,
    // Hold
    STATE_H,
    // None
    STATE_N
};

class MessageForger
{
public:

    // Message types
    enum {
        // Launch the Singhal algorithm
        LAUNCH,

        // First message type sent from/to clients and the server
        HELLO,

        // Request for aquiring critical section
        REQUEST,

        // This message contains token
        TOKEN,

        // Request for writing a file in host OS (critical section operation)
        WRITE,

        // Somehow an error occurred
        ERROR,

        // Our experiment is over
        STOP
    };

    // Parse-level errors
    enum {
        // The JSON string is syntactically malformed.
        PARSE_ERROR,

        // Required keys was not found in main JSON object.
        REQUIRED_KEYS_IS_MISSING,

        // Obviously invalid values for some keys
        INVALID_CONTENT
    };

    // Special site identifiers
    enum{
        // Site ID of this program
        CONTROLER_SITE_ID = -1,

        // An invalid site ID representing that site ID has not been set.
        INVALID_SITE_ID = -2
    };

    // Message model
    class Message {
    public:
        int errorCode;
        int type;
        int sender;
        int receiver;
        int sitesCount;
        int requesterSN;
        bool initialTockenHolder;
        string fileName;
        string fileContent;
        int *TSN;
        int *TSV;
        int *SN;
        int *SV;

        Message(int type, int sender, int receiver):
            type(type),
            sender(sender),
            receiver(receiver){}
    };

    // Method for parsing a JSON string
    static Message parse(string msgstr);

    static Message parse(char *msgstr);

    // Method for serializing a message object into a JSON string.
    // For more compatibility with native methods, return type of this serializer is const char *
    static const char *serialize(Message message);

    static vector<string> splitMessages(string msgstr);
};

#endif // SINGHALCOMMONPARSER_H
