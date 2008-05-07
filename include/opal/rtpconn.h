/*
 * rtpconn.h
 *
 * Connection abstraction
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2007 Post Increment
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 19424 $
 * $Author: csoutheren $
 * $Date: 2008-02-08 17:24:10 +1100 (Fri, 08 Feb 2008) $
 */

#ifndef __OPAL_RTPCONN_H
#define __OPAL_RTPCONN_H

#include <opal/buildopts.h>

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/connection.h>
#include <opal/mediatype.h>


class OpalRTPEndPoint;

//#ifdef HAS_LIBZRTP
//#ifndef __ZRTP_TYPES_H__
//struct zrtp_conn_ctx_t;
//#endif
//#endif

/** Class for carrying media session information
  */
class OpalMediaSession : public PObject
{
  PCLASSINFO(OpalMediaSession, PObject);
  public:
    OpalMediaSession(const OpalMediaType & _mediaType);
    OpalMediaSession(const OpalMediaSession & _obj);

    PObject * Clone() const { return new OpalMediaSession(*this); }

    OpalMediaType mediaType;     // media type for session
    bool autoStartReceive;       // if true, this session should receive data when the call is started
    bool autoStartTransmit;      // if true, this session  should transmit data when the call is started
    unsigned sessionId;          // unique session ID

    RTP_Session * rtpSession;    // RTP session
};

/**This class manages the RTP sessions for an OpalRTPConnection
 */
class OpalRTPSessionManager : public PObject
{
  PCLASSINFO(OpalRTPSessionManager , PObject);

  public:
  /**@name Construction */
  //@{
    /**Construct new session manager database.
      */
    OpalRTPSessionManager();
    ~OpalRTPSessionManager();
  //@}

    void CopyFromMaster(const OpalRTPSessionManager & sm);
    void CopyToMaster(OpalRTPSessionManager & sm);

    /**
       Initialise the autostart options within the session
      */
    void Initialise(
      OpalRTPConnection & conn, 
      OpalConnection::StringOptions * stringOptions
    );

    /**
       Initialise the autostart options for a session
      */
    unsigned AutoStartSession(
      unsigned sessionID,        ///<  Session ID to use.
      const OpalMediaType & mediaType,
      bool autoStartReceive, 
      bool autoStartTransmit
    );

  /**@name Operations */
  //@{
    /**Use an RTP session for the specified ID.

       If this function returns a non-null value, then the ReleaseSession()
       function MUST be called or the session is never deleted for the
       lifetime of the session manager.

       If there is no session of the specified ID, then you MUST call the
       AddSession() function with a new RTP_Session. The mutex flag is left
       locked in this case. The AddSession() expects the mutex to be locked
       and unlocks it automatically.
      */
    RTP_Session * UseSession(
      unsigned sessionID    ///<  Session ID to use.
    );

    /**Add an RTP session for the specified ID.

       This function MUST be called only after the UseSession() function has
       returned NULL. The mutex flag is left locked in that case. This
       function expects the mutex to be locked and unlocks it automatically.
      */
    void AddSession(
      RTP_Session * session,          ///<  Session to add.
      const OpalMediaType & mediaType ///< initial media type for this session
    );

    /**Release the session. If the session ID is not being used any more any
       clients via the UseSession() function, then the session is deleted.
     */
    void ReleaseSession(
      unsigned sessionID,    ///<  Session ID to release.
      PBoolean clearAll = PFalse  ///<  Clear all sessions with that ID
    );

    /**Get a session for the specified ID.
       Unlike UseSession, this does not increment the usage count on the
       session so may be used to just gain a pointer to an RTP session.
     */
    RTP_Session * GetSession(
      unsigned sessionID    ///<  Session ID to get.
    ) const;

    void SetCleanup(bool v) { m_cleanupOnDelete = v; }

  //@}
    PMutex & GetMutex() { return m_mutex; }

  protected:
    void SetOldOptions(unsigned channelId, const OpalMediaType & mediaType, bool rx, bool tx);

    PMutex m_mutex;
    bool m_initialised;
    bool m_cleanupOnDelete;
    PDICTIONARY(SessionDict, POrdinalKey, OpalMediaSession);
    SessionDict sessions;

  private:
    OpalRTPSessionManager (const OpalRTPSessionManager &) { }
    OpalRTPSessionManager & operator=(const OpalRTPSessionManager &) { return *this; }

};

typedef OpalRTPSessionManager RTP_SessionManager;

/**This is the base class for OpalConnections that use RTP sessions, 
   such as H.323 and SIPconnections to an endpoint.
 */
class OpalRTPConnection : public OpalConnection
{
  PCLASSINFO(OpalRTPConnection, OpalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    OpalRTPConnection(
      OpalCall & call,                         ///<  Owner calll for connection
      OpalRTPEndPoint & endpoint,              ///<  Owner endpoint for connection
      const PString & token,                   ///<  Token to identify the connection
      unsigned options = 0,                    ///<  Connection options
      OpalConnection::StringOptions * stringOptions = NULL     ///< more complex options
    );  

    /**Destroy connection.
     */
    ~OpalRTPConnection();


  /**@name RTP Session Management */
  //@{
    /**Get an RTP session for the specified ID.
       If there is no session of the specified ID, NULL is returned.
      */
    virtual RTP_Session * GetSession(
      unsigned sessionID    ///<  RTP session number
    ) const;

    /**Use an RTP session for the specified ID.
       This will find a session of the specified ID and increment its
       reference count. Multiple OpalRTPStreams use this to indicate their
       usage of the RTP session.

       If this function is used, then the ReleaseSession() function MUST be
       called or the session is never deleted for the lifetime of the Opal
       connection.

       If there is no session of the specified ID one is created.

       The type of RTP session that is created will be compatible with the
       transport. At this time only IP (RTp over UDP) is supported.
      */
    virtual RTP_Session * UseSession(
      unsigned sessionID
    );
    virtual RTP_Session * UseSession(
      const OpalTransport & transport,  ///<  Transport of signalling
      unsigned sessionID,               ///<  RTP session number
      const OpalMediaType & mediatype,  ///<  media type
      RTP_QOS * rtpqos = NULL           ///<  Quiality of Service information
    );

    /**Release the session.
       If the session ID is not being used any more any clients via the
       UseSession() function, then the session is deleted.
     */
    virtual void ReleaseSession(
      unsigned sessionID,    ///<  RTP session number
      PBoolean clearAll = PFalse  ///<  Clear all sessions
    );

    /**Create and open a new RTP session.
       The type of RTP session that is created will be compatible with the
       transport. At this time only IP (RTp over UDP) is supported.
      */
    virtual RTP_Session * CreateSession(
      const OpalTransport & transport,
      unsigned sessionID,
      RTP_QOS * rtpqos
    );

  //@}

  //@{
    /** Return PTrue if the remote appears to be behind a NAT firewall
    */
    virtual PBoolean RemoteIsNAT() const
    { return remoteIsNAT; }

    /**Determine if the RTP session needs to accommodate a NAT router.
       For endpoints that do not use STUN or something similar to set up all the
       correct protocol embeddded addresses correctly when a NAT router is between
       the endpoints, it is possible to still accommodate the call, with some
       restrictions. This function determines if the RTP can proceed with special
       NAT allowances.

       The special allowance is that the RTP code will ignore whatever the remote
       indicates in the protocol for the address to send RTP data and wait for
       the first packet to arrive from the remote and will then proceed to send
       all RTP data back to that address AND port.

       The default behaviour checks the values of the physical link
       (localAddr/peerAddr) against the signaling address the remote indicated in
       the protocol, eg H.323 SETUP sourceCallSignalAddress or SIP "To" or
       "Contact" fields, and makes a guess that the remote is behind a NAT router.
     */
    virtual PBoolean IsRTPNATEnabled(
      const PIPSocket::Address & localAddr,   ///< Local physical address of connection
      const PIPSocket::Address & peerAddr,    ///< Remote physical address of connection
      const PIPSocket::Address & signalAddr,  ///< Remotes signaling address as indicated by protocol of connection
      PBoolean incoming                       ///< Incoming/outgoing connection
    );
  //@}

    /**Attaches the RFC 2833 handler to the media patch
       This method may be called from subclasses, e.g. within
       OnPatchMediaStream()
      */
    virtual void AttachRFC2833HandlerToPatch(PBoolean isSource, OpalMediaPatch & patch);

    virtual PBoolean SendUserInputTone(
      char tone,        ///<  DTMF tone code
      unsigned duration = 0  ///<  Duration of tone in milliseconds
    );

    /**Meda information structure for GetMediaInformation() function.
      */
    struct MediaInformation {
      MediaInformation() { 
        rfc2833  = RTP_DataFrame::IllegalPayloadType; 
        ciscoNSE = RTP_DataFrame::IllegalPayloadType; 
      }

      OpalTransportAddress data;           ///<  Data channel address
      OpalTransportAddress control;        ///<  Control channel address
      RTP_DataFrame::PayloadTypes rfc2833; ///<  Payload type for RFC2833
      RTP_DataFrame::PayloadTypes ciscoNSE; ///<  Payload type for RFC2833
    };

    /**Get information on the media channel for the connection.
       The default behaviour checked the mediaTransportAddresses dictionary
       for the session ID and returns information based on that. It also uses
       the rfc2833Handler variable for that part of the info.

       It is up to the descendant class to assure that the mediaTransportAddresses
       dictionary is set correctly before OnIncomingCall() is executed.
     */
    virtual PBoolean GetMediaInformation(
      unsigned sessionID,     ///<  Session ID for media channel
      MediaInformation & info ///<  Information on media channel
    ) const;

    /**See if the media can bypass the local host.

       The default behaviour returns PTrue if the session is audio or video.
     */
    virtual PBoolean IsMediaBypassPossible(
      unsigned sessionID                  ///<  Session ID for media channel
    ) const;

    /**Create a new media stream.
       This will create a media stream of an appropriate subclass as required
       by the underlying connection protocol. For instance H.323 would create
       an OpalRTPStream.

       The sessionID parameter may not be needed by a particular media stream
       and may be ignored. In the case of an OpalRTPStream it us used.

       Note that media streams may be created internally to the underlying
       protocol. This function is not the only way a stream can come into
       existance.
     */
    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      PBoolean isSource                        ///<  Is a source stream
    );

    /**Overrides from OpalConnection
      */
    virtual void OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch);

    virtual void SetSecurityMode(const PString & v)
    { securityMode = v; }

    virtual PString GetSecurityMode() const 
    { return securityMode; }

    virtual void * GetSecurityData();         
    virtual void SetSecurityData(void *data); 

    void OnMediaCommand(OpalMediaCommand & command, INT extra);

    PDECLARE_NOTIFIER(OpalRFC2833Info, OpalRTPConnection, OnUserInputInlineRFC2833);
    PDECLARE_NOTIFIER(OpalRFC2833Info, OpalRTPConnection, OnUserInputInlineCiscoNSE);

  protected:
    PString securityMode;
    void * securityData;
    OpalRTPSessionManager m_rtpSessions;
    OpalRFC2833Proto * rfc2833Handler;
    OpalRFC2833Proto * ciscoNSEHandler;

    PBoolean remoteIsNAT;
    PBoolean useRTPAggregation;
};


class RTP_UDP;

class OpalSecurityMode : public PObject
{
  PCLASSINFO(OpalSecurityMode, PObject);
  public:
    virtual RTP_UDP * CreateRTPSession(
#if OPAL_RTP_AGGREGATE
      PHandleAggregator * _aggregator,   ///< handle aggregator
#endif
      unsigned id,                       ///< Session ID for RTP channel
      PBoolean remoteIsNAT,              ///< PTrue is remote is behind NAT
      OpalConnection & connection	 ///< Connection creating session (may be needed by secure connections)
    ) = 0;
    virtual PBoolean Open() = 0;
};

#endif 

