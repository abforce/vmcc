#include "messageforger.h"

MessageForger::Message MessageForger::parse(string message)
{
    try{
        // Root literal object of our received JSON string
        jsonxx::Object root;

        // Try to parse the raw string
        if(!root.parse(message)){
            throw PARSE_ERROR;
        }

        // Objects with no 'type' and 'sender' and 'receiver' fields contained are not acceptable
        if (!root.has<jsonxx::Number>(KEY_TYPE)
                || !root.has<jsonxx::Number>(KEY_SENDER)
                || !root.has<jsonxx::Number>(KEY_RECEIVER)){
            throw REQUIRED_KEYS_IS_MISSING;
        }

        // Extract 'type' field
        int type = root.get<jsonxx::Number>(KEY_TYPE);

        // Only the following types are acceptable.
        if(type != HELLO && type != REQUEST && type != TOKEN && type != LAUNCH && type != WRITE && type != STOP){
            throw INVALID_CONTENT;
        }

        // Extract 'sender' and 'receiver' fields
        int sender = root.get<jsonxx::Number>(KEY_SENDER);
        int receiver = root.get<jsonxx::Number>(KEY_RECEIVER);

        // Cut off site identifiers lower than CONTROLER_SITE_ID
        if(sender < CONTROLER_SITE_ID || receiver < CONTROLER_SITE_ID){
            throw INVALID_CONTENT;
        }

        // Message object. To be returned
        Message message(type, sender, receiver);

        // Each type may have its own dedicated fields, so leave it off to their own.
        switch (type) {
        case HELLO:
            if(sender == CONTROLER_SITE_ID){
                message.initialTockenHolder = root.get<jsonxx::Boolean>(KEY_INITIAL_TOKEN_HOLDER);
                message.sitesCount = root.get<jsonxx::Number>(KEY_SITES_COUNT);
            }
            break;
        case REQUEST:{
            message.requesterSN = root.get<jsonxx::Number>(KEY_REQUESTER_SN);
            jsonxx::Array sn = root.get<jsonxx::Array>(KEY_SN);
            jsonxx::Array sv = root.get<jsonxx::Array>(KEY_SV);
            int count = sn.size();
            message.SN = new int[count];
            message.SV = new int[count];
            for(int i = 0; i < count; i += 1){
                message.SN[i] = sn.get<jsonxx::Number>(i);
                message.SV[i] = sv.get<jsonxx::Number>(i);
            }
            break;
        }
        case TOKEN:{
            jsonxx::Array tsn = root.get<jsonxx::Array>(KEY_TSN);
            jsonxx::Array tsv = root.get<jsonxx::Array>(KEY_TSV);
            jsonxx::Array sn = root.get<jsonxx::Array>(KEY_SN);
            jsonxx::Array sv = root.get<jsonxx::Array>(KEY_SV);
            int count = tsn.size();
            message.TSN = new int[count];
            message.TSV = new int[count];
            message.SN = new int[count];
            message.SV = new int[count];
            for(int i = 0; i < count; i += 1){
                message.TSN[i] = tsn.get<jsonxx::Number>(i);
                message.TSV[i] = tsv.get<jsonxx::Number>(i);
                message.SN[i] = sn.get<jsonxx::Number>(i);
                message.SV[i] = sv.get<jsonxx::Number>(i);
            }
            break;
        }
        case LAUNCH:
            // Do nothing
            break;
        case WRITE:
            message.fileName = (char *) root.get<jsonxx::String>(KEY_FILE_NAME).c_str();
            message.fileContent = (char *) root.get<jsonxx::String>(KEY_FILE_CONTENT).c_str();
            break;
        case STOP:
            // Do nothing
            break;
        }

        // Kick it out!
        return message;

    } catch(int err){
        // Oh!, Something went wrong, return a message of type error suplied with error code.
        Message message(ERROR, INVALID_SITE_ID, INVALID_SITE_ID);
        message.errorCode = err;
        return message;
    }

}

MessageForger::Message MessageForger::parse(char *message){
    return parse(string(message));
}

const char * MessageForger::serialize(MessageForger::Message message)
{
    jsonxx::Object root;
    root << KEY_TYPE << message.type;
    root << KEY_SENDER << message.sender;
    root << KEY_RECEIVER << message.receiver;

    switch(message.type){
    case HELLO:
        if(message.sender == CONTROLER_SITE_ID){
            root << KEY_INITIAL_TOKEN_HOLDER << message.initialTockenHolder;
            root << KEY_SITES_COUNT << message.sitesCount;
        }
        break;
    case REQUEST:{
        root << KEY_REQUESTER_SN << message.requesterSN;
        jsonxx::Array sn;
        jsonxx::Array sv;
        for(int i = 0; i < message.sitesCount; i += 1){
            sn << message.SN[i];
            sv << message.SV[i];
        }
        root << KEY_SN << sn;
        root << KEY_SV << sv;
        break;
    }
    case TOKEN:{
        jsonxx::Array tsn;
        jsonxx::Array tsv;
        jsonxx::Array sn;
        jsonxx::Array sv;

        for(int i = 0; i < message.sitesCount; i += 1){
            tsn << message.TSN[i];
            tsv << message.TSV[i];
            sn << message.SN[i];
            sv << message.SV[i];
        }
        root << KEY_TSN << tsn;
        root << KEY_TSV << tsv;
        root << KEY_SN << sn;
        root << KEY_SV << sv;
        break;
    }
    case LAUNCH:
        // Do nothing
        break;
    case WRITE:
        root << KEY_FILE_NAME << message.fileName;
        root << KEY_FILE_CONTENT << message.fileContent;
        break;
    case STOP:
        // Do nothing
        break;
    }

    string json = root.json();
    char *str = new char[json.length() + 1];
    strcpy(str, json.c_str());
    return str;
}
