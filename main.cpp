#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libwebsockets.h>
#include <wiringPi.h>
#include "guidance.h"
#include "hardware.h"

static int callback_http(struct lws *wsi,
                         enum lws_callback_reasons reason, void *user,
                         void *in, size_t len)
{
    return 0;
}
static int callback_roombot(struct lws *wsi,
                            enum lws_callback_reasons reason,
                            void *user, void *in, size_t len)
{
    if(reason == LWS_CALLBACK_ESTABLISHED)
    {
        printf("connection established\n");
    }
    else if(reason == LWS_CALLBACK_RECEIVE)
    {
        //if we're vaccuming, we don't want to be manually driving at all
        if(isVacOn())
        {
            return 0;
        }
        //decode the client response (super easy, not reall much to decode)
        int turnVal = atoi((char *)in);
        switch(turnVal)
        {
            case 1:
                //stop
                digitalWrite(MOTORS, 0);
                break;
            case 2:
                //drive forward
                digitalWrite(MOTORS, 0);
                digitalWrite(MOTOR_L, 0);
                digitalWrite(MOTOR_R, 0);
                digitalWrite(MOTORS, 1);
                break;
            case 3:
                //reverse
                digitalWrite(MOTORS, 0);
                digitalWrite(MOTOR_L, 1);
                digitalWrite(MOTOR_R, 1);
                digitalWrite(MOTORS, 1);
                break;
            case 4: 
                //turn left
                digitalWrite(MOTORS, 0);
                digitalWrite(MOTOR_L, 1);
                digitalWrite(MOTOR_R, 0);
                digitalWrite(MOTORS, 1);
                break;
            case 5:
                //turn right
                digitalWrite(MOTORS, 0);
                digitalWrite(MOTOR_L, 0);
                digitalWrite(MOTOR_R, 1);
                digitalWrite(MOTORS, 1);
                break;
            case 6:
                //toggle vaccuming mode
                toggleVac();
                break;
        }
    }
    else if(reason == LWS_CALLBACK_SERVER_WRITEABLE)
    {
        char *img = base64img();
        if(img == NULL)
        {
            return -1;
        }
        // create a buffer to hold our response
        // it has to have some pre and post padding. You don't need to care
        // what comes there, libwebsockets will do everything for you. For more info see
        // http://git.warmcat.com/cgi-bin/cgit/libwebsockets/tree/lib/libwebsockets.h#n597
        
        //encode everything in JSON
        char* json = (char*) malloc(strlen(img) + 255);
        if(json == NULL)
        {
            return -1;
        }
        sprintf(json, "{\"webcam\":\"%s\",\"vac\":%s,\"distance\":%d}", img, c_isVacOn(), readSensor());

        unsigned char *buf = (unsigned char*) malloc(LWS_SEND_BUFFER_PRE_PADDING + strlen(json) + LWS_SEND_BUFFER_POST_PADDING);
        if(buf == NULL)
        {
            return -1;
        }

        //write image to client
        strcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, json);
        lws_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], strlen(json), LWS_WRITE_TEXT);
        free(img);
        free(json);
        free(buf);
    }
    
    return 0;
}
static struct lws_protocols protocols[] = {
    /* first protocol must always be HTTP handler */
    {
        "http-only",   // name
        callback_http, // callback
        0             // per_session_data_size
    },
    {
        "roombot-protocol", // protocol name - very important!
        callback_roombot,   // callback
        0,                         // we don't use any per session data
        4096
    },
    {
        NULL, NULL, 0   /* End of list */
    }
};
int main(void) 
{
    if(initHardware() != 0)
    {
        fprintf(stderr, "WiringPi init failed\n");
        return -1;
    }
    startCapture();
    sleep(1);
    // server url will be http://localhost:9000
    struct lws_context *context;
    struct lws_context_creation_info  info;
    memset(&info, 0, sizeof info);
    info.port = 9000;
    info.iface = NULL;
    info.protocols = protocols;
    info.ssl_cert_filepath = NULL;
    info.ssl_ca_filepath = NULL;
    info.gid = -1;
    info.uid = -1;
    info.options = 0;
    info.max_http_header_pool = 16;
    info.timeout_secs = 60;
    info.server_string = "roombot";
    // create libwebsocket context representing this server
    context = lws_create_context(&info);

    if (context == NULL) 
    {
        fprintf(stderr, "libwebsocket init failed\n");
        return -1;
    }
   
    printf("starting server...\n");
   
    // infinite loop, to end this server send SIGTERM. (CTRL+C)
    while (1) 
    {
        lws_service(context, 100);
        lws_callback_on_writable_all_protocol(context, &protocols[1]);
    }
   
    lws_context_destroy(context);
    stopCapture();
    return 0;
}
