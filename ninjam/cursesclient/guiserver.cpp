
#include <fstream>

#include "guiserver.h"
#include "../njclient.h"


extern NJClient *g_client ;


IPageGenerator* GuiServer::onConnection(JNL_HTTPServ *serv , int port)
{
	char* event = serv->get_request_file() ; event++ ;
	char* data = serv->get_request_parm(DATA_KEY) ;
	serv->set_reply_header(HTTP_REPLY_SERVER) ;

	// via remote script (poll)
	if (!strcmp(event , PING_SIGNAL))
	{
//printf("%dload.js" , N++) ;

		// the remote load script that contains the ninjam:// link that triggered this app
		//		polls us until we reply that the js client can be requested confidently
		serv->set_reply_header(HTTP_REPLY_JS) ;
		serv->set_reply_string(HTTP_REPLY_200) ; serv->send_reply() ;
		return new MemPageGenerator(strdup(PONG_SIGNAL)) ;
	}

	// via remote script "loadGui()"
	if (!strcmp(event , GET_SIGNAL))
	{
		serv->set_reply_header(HTTP_REPLY_HTML) ;
		serv->set_reply_string(HTTP_REPLY_200) ; serv->send_reply() ;

		// read CLIENT_HTML file and return failure message on i/o error
		std::ifstream clientHtmlIfs(CLIENT_HTML) ; std::stringstream clientHtmlSs ;
		if (!clientHtmlIfs.good()) return new MemPageGenerator(strdup("file not found")) ;

		// else return main client gui html
		clientHtmlSs << clientHtmlIfs.rdbuf() ;
		return new MemPageGenerator(strdup(clientHtmlSs.str().c_str())) ;
	}

	// the remaining cases process local gui input and output events
	serv->set_reply_header(HTTP_REPLY_TEXT) ;
	serv->set_reply_string(HTTP_REPLY_200) ; serv->send_reply() ;
	std::stringstream outputJSON ; outputJSON << "{" ;

	if (!strcmp(event , INIT_SIGNAL)) returnInitEvents(&outputJSON) ;
	else if (!strcmp(event , METROMUTE_SIGNAL))
		{ handleMetroMuteEvent(data) ; returnMetroMuteEvent(&outputJSON) ; }
	else if (!strcmp(event , CHAT_SIGNAL)) handleChatEvent(data) ;

	returnCoreEvents(&outputJSON) ;
	const std::string& outStr = outputJSON.str() ; unsigned int len = outStr.length() ;
	char out[len + 1] ; sprintf(out , outStr.c_str()) ; out[len - 1] = '}' ;
	return new MemPageGenerator(strdup(out)) ;
}

void GuiServer::returnInitEvents(std::stringstream* outputJSON)
{
	returnBpmEvent(outputJSON) ;
	returnBpiEvent(outputJSON) ;
	returnMetroMuteEvent(outputJSON) ;
}

void GuiServer::returnBpmEvent(std::stringstream* out)
	{ *out << BPM_SIGNAL << ":" << g_client->GetActualBPM() << "," ; }

void GuiServer::returnBpiEvent(std::stringstream* out)
	{ *out << BPI_SIGNAL << ":" << g_client->GetBPI() << "," ; }

//TODO: as we are async - let's be specific and use eventData here
void GuiServer::handleMetroMuteEvent(char* data)
	{ g_client->config_metronome_mute = !g_client->config_metronome_mute ; }

void GuiServer::returnMetroMuteEvent(std::stringstream* out)
	{ *out << METROMUTE_SIGNAL << ":" << ((g_client->config_metronome_mute)? "1" : "0") << "," ; }

void GuiServer::handleChatEvent(char* data) { g_client->ChatMessage_Send("MSG" , data) ; }

//void handleChatPvtEvent(char* destFullUserName , char* chatMsg) { g_client->ChatMessage_Send("PRIVMSG" , destFullUserName , chatMsg) ; }

void GuiServer::returnCoreEvents(std::stringstream* out)
{
	// interval progress
	int bpi = g_client->GetBPI() ; int pos , len ; g_client->GetPosition(&pos , &len) ;
	*out << PROGRESS_SIGNAL << ":" << ((pos * bpi) / len) << "," ;

/*TODO: beat message
	char output[11];
	snprintf(output, sizeof(output), "%d", (pos*bpi)/len);
	Glib::ustring beatmsg = output;
	beatmsg += "/";
	snprintf(output, sizeof(output), "%d", bpi);
	beatmsg += output;
	beatmsg += " @ ";
	snprintf(output, sizeof(output), "%.1f", g_client->GetActualBPM());
	beatmsg += output;
	beatmsg += _(" BPM ");
	progressbar1->set_text(beatmsg);
*//* master VU
	float value = VAL2DB(g_client->GetOutputPeak());
	progressbar_master->set_fraction((value+120)/140);
	snprintf(output, sizeof(output), "%.2f dB", value);
	progressbar_master->set_text(output);
*//* user VUs
	vbox_local->update_VUmeters();
	vbox_remote->update_VUmeters();
	if (g_client->HasUserInfoChanged())
	vbox_remote->update();
*/
}
