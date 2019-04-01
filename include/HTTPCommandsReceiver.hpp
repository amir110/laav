/* 
 * Created (25/04/2017) by Paolo-Pr.
 * This file is part of Live Asynchronous Audio Video Library.
 *
 * Live Asynchronous Audio Video Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Live Asynchronous Audio Video Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Live Asynchronous Audio Video Library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef HTTPCOMMANDSRECEIVER_HPP_INCLUDED
#define HTTPCOMMANDSRECEIVER_HPP_INCLUDED

#include "EventsManager.hpp"

namespace laav
{

class HTTPCommandsReceiver : public EventsProducer, public Medium
{

public:

    HTTPCommandsReceiver(SharedEventsCatcher eventsCatcher, std::string address, unsigned int port, const std::string& id = ""):
        EventsProducer::EventsProducer(eventsCatcher),
        Medium(id),        
        mErrno(0),        
        mAddress(address),
        mPort(port)
    {
        std::string location = "/commands";
        if (!makeHTTPServerPollable(mAddress, location, mPort))
        {
            mErrno = errno;
            return;
        }

        observeHTTPEventsOn(mAddress, mPort);
        mInputStatus  = READY;
    }

    std::map<std::string, std::string>& receivedCommands()
    {
        Medium::mOutputStatus = NOT_READY; 

        if (mCommands.size() == 0)
            throw MediumException(OUTPUT_NOT_READY);        
        
        if (Medium::mInputStatus ==  PAUSED)
            throw MediumException(MEDIUM_PAUSED);        
        
        Medium::mOutputStatus = READY; 
        
        return mCommands;
    }

    void clearCommands()
    {
        mCommands.clear();
    }

    int getErrno() const
    {
        return mErrno;
    }

private:

    void hTTPDisconnectionCallBack(struct evhttp_connection* clientConnection)
    {
    }

    void hTTPConnectionCallBack(struct evhttp_request* clientRequest,
                                struct evhttp_connection* clientConnection)
    {
        struct evbuffer* inputBuf = evhttp_request_get_input_buffer(clientRequest);
        std::string data(evbuffer_get_length(inputBuf), ' ');
        evbuffer_copyout(inputBuf, &data[0], data.size());
        std::vector<std::string> tokens = split(data, '&');
        mCommands.clear();
        unsigned int q;
        for (q = 0; q < tokens.size(); q++)
        {
            std::vector<std::string> paramAndVal = split(tokens[q], '=');
            if (paramAndVal.size() == 2)
            {
                mCommands[paramAndVal[0]] = paramAndVal[1];
            }
        };
        struct evbuffer* buf = evbuffer_new();
        if (buf == NULL)
            printAndThrowUnrecoverableError("buf == NULL");
        evbuffer_add_printf(buf, "OK\n");
        evhttp_send_reply(clientRequest, HTTP_OK, "OK", buf);
        evbuffer_free(buf);
    }

    int mErrno;
    std::map<std::string, std::string> mCommands;
    std::map<struct evhttp_connection*, struct evhttp_request*> mClientConnectionsAndRequests;
    std::string mAddress;
    unsigned int mPort;

};

}

#endif // HTTPCOMMANDSRECEIVER_HPP_INCLUDED
