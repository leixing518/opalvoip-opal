/*
 * mediastrm.h
 *
 * Media Stream classes
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef __OPAL_MEDIASTRM_H
#define __OPAL_MEDIASTRM_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptclib/delaychan.h>

#include <opal/buildopts.h>
#include <opal/mediafmt.h>
#include <opal/mediacmd.h>
#include <ptlib/safecoll.h>
#include <ptclib/guid.h>


class RTP_Session;
class OpalMediaPatch;
class OpalLine;
class OpalConnection;
class OpalMediaStatistics;


/**This class describes a media stream as used in the OPAL system. A media
   stream is the channel through which media data is trasferred between OPAL
   entities. For example, data being sent to an RTP session over a network
   would be through a media stream.
  */
class OpalMediaStream : public PSafeObject
{
    PCLASSINFO(OpalMediaStream, PSafeObject);
  protected:
  /**@name Construction */
  //@{
    /**Construct a new media stream.
      */
    OpalMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                        ///<  Is a source stream
    );

  public:
    /**Destroy the media stream.
       Make sure the patch, if present, has been stopped and deleted.
      */
    ~OpalMediaStream();
  //@}

  public:
  /**@name Overrides from PObject */
  //@{
    /**Standard stream print function.
       The PObject class has a << operator defined that calls this function
       polymorphically.
      */
    void PrintOn(
      ostream & strm    ///<  Stream to output text representation
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Get the currently selected media format.
       The media data format is a string representation of the format being
       transferred by the media channel. It is typically a value as provided
       by the RTP_PayloadType class.

       The default behaviour simply returns the member variable "mediaFormat".
      */
    virtual OpalMediaFormat GetMediaFormat() const;

    /**Update the media format. This can be used to adjust the
       parameters of a codec at run time. Note you cannot change the basic
       media format, eg change GSM0610 to G.711, only options for that
       format, eg 6k3 mode to 5k3 mode in G.723.1.

       The default behaviour updates the mediaFormat member variable and
       pases the value on to the OpalMediaPatch.
      */
    virtual PBoolean UpdateMediaFormat(
      const OpalMediaFormat & mediaFormat  ///<  New media format
    );

    /**Execute the command specified to the transcoder. The commands are
       highly context sensitive, for example VideoFastUpdate would only apply
       to a video transcoder.

       The default behaviour passes the command on to the OpalMediaPatch.
      */
    virtual PBoolean ExecuteCommand(
      const OpalMediaCommand & command    ///<  Command to execute.
    );

    /**Set a notifier to receive commands generated by the transcoder. The
       commands are highly context sensitive, for example VideoFastUpdate
       would only apply to a video transcoder.

       The default behaviour passes the command on to the OpalMediaPatch and
       sets the member variable commandNotifier.
      */
    virtual void SetCommandNotifier(
      const PNotifier & notifier    ///<  Command to execute.
    );

    /**Open the media stream using the media format.

       The default behaviour simply sets the isOpen variable to true.
      */
    virtual PBoolean Open();

    /**Start the media stream.

       The default behaviour calls Resume() on the associated OpalMediaPatch
       thread if it was suspended.
      */
    virtual PBoolean Start();

    /**Close the media stream.

       The default does nothing.
      */
    virtual PBoolean Close();

    /**Callback that is called on the source stream once the media patch has started.
       The default behaviour calls OpalConnection::OnMediaPatchStart()
      */
    virtual void OnPatchStart();

    /**Callback that is called on the source stream once the media patch has started.
       The default behaviour calls OpalConnection::OnMediaPatchStop()
      */
    virtual void OnPatchStop();

    /**Write a list of RTP frames of data to the sink media stream.
       The default behaviour simply calls WritePacket() on each of the
       elements in the list.
      */
    virtual PBoolean WritePackets(
      RTP_DataFrameList & packets
    );

    /**Read an RTP frame of data from the source media stream.
       The default behaviour simply calls ReadData() on the data portion of the
       RTP_DataFrame and sets the frames timestamp and marker from the internal
       member variables of the media stream class.
      */
    virtual PBoolean ReadPacket(
      RTP_DataFrame & packet
    );

    /**Write an RTP frame of data to the sink media stream.
       The default behaviour simply calls WriteData() on the data portion of the
       RTP_DataFrame and and sets the internal timestamp and marker from the
       member variables of the media stream class.
      */
    virtual PBoolean WritePacket(
      RTP_DataFrame & packet
    );

    /**Read raw media data from the source media stream.
       The default behaviour simply calls ReadPacket() on the data portion of the
       RTP_DataFrame and sets the frames timestamp and marker from the internal
       member variables of the media stream class.
      */
    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour calls WritePacket() on the data portion of the
       RTP_DataFrame and and sets the internal timestamp and marker from the
       member variables of the media stream class.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );

    /**Pushes a frame to the patch
      */
    bool PushPacket(
      RTP_DataFrame & packet
    );

    /**Set the data size in bytes that is expected to be used. Some media
       streams can make use of this information to perform optimisations.

       The default behaviour does nothing.
      */
    virtual PBoolean SetDataSize(
      PINDEX dataSize  ///<  New data size
    );

    /**Get the data size in bytes that is expected to be used. Some media
       streams can make use of this information to perform optimisations.
      */
    PINDEX GetDataSize() const { return defaultDataSize; }

    /**Indicate if the media stream is synchronous.
       If this returns true then the media stream will block of the amount of
       time it takes to annunciate the data. For example if the media stream
       is over a sound card, and 480 bytes of data are to be written it will
       take 30 milliseconds to complete.
      */
    virtual PBoolean IsSynchronous() const = 0;
	
    /**Indicate if the media stream requires a OpalMediaPatch thread (active patch).
       The default behaviour returns true.
      */
    virtual PBoolean RequiresPatchThread() const;

    /**Enable jitter buffer for the media stream.

       The default behaviour does nothing.
      */
    virtual void EnableJitterBuffer() const;
  //@}

  /**@name Member variable access */
  //@{
    /** Get the owner connection.
     */
    OpalConnection & GetConnection() const { return connection; }

    /**Determine of media stream is a source or a sink.
      */
    bool IsSource() const { return isSource; }

    /**Determine of media stream is a source or a sink.
      */
    bool IsSink() const { return !isSource; }

    /**Get the session number of the stream.
     */
    unsigned GetSessionID() const { return sessionID; }

    /**  Get the ID associated with this stream. Used for detecting two 
      *  the streams associated with a bidirectional media channel
      */
    PString GetID() const { return identifier; }

    /**Get the timestamp of last read.
      */
    unsigned GetTimestamp() const { return timestamp; }

    /**Set timestamp for next write.
      */
    void SetTimestamp(unsigned ts) { timestamp = ts; }

    /**Get the marker bit of last read.
      */
    bool GetMarker() const { return marker; }

    /**Set marker bit for next write.
      */
    void SetMarker(bool m) { marker = m; }

    /**Get the paused state for writing.
      */
    bool IsPaused() const { return paused; }

    /**Set the paused state for writing.
      */
    void SetPaused(bool p) { paused = p; }

    /**Returns true if the media stream is open.
      */
    bool IsOpen() { return isOpen; }
    
    /**Set the patch thread that is using this stream.
      */
    virtual PBoolean SetPatch(
      OpalMediaPatch * patch  ///<  Media patch thread
    );

    /**Remove the patch thread that is using this stream.
       This function is useful in case of streams which can be accessed by
       multiple instances of OpalMediaPatch.

       The default behaviour simply sets patchThread to NULL.
    */
    virtual void RemovePatch(OpalMediaPatch * patch);

    /**Get the patch thread that is using the stream.
      */
    OpalMediaPatch * GetPatch() const { return mediaPatch; }

    /**Add a filter to the owning patch safely.
      */
    void AddFilter(const PNotifier & Filter, const OpalMediaFormat & Stage =  OpalMediaFormat());

    /**Remove a filter from the owning patch safely.
      */
    bool RemoveFilter(const PNotifier & Filter, const OpalMediaFormat & Stage);

#ifdef OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics) const;
#endif
  //@}

  protected:

    virtual void BitRateLimit (PINDEX byteCount, PBoolean mayDelay);

    OpalConnection & connection;
    unsigned         sessionID;
    PString          identifier;
    OpalMediaFormat  mediaFormat;
    bool             paused;
    bool             isSource;
    bool             isOpen;
    PINDEX           defaultDataSize;
    unsigned         timestamp;
    bool             marker;
    unsigned         mismatchedPayloadTypes;

    OpalMediaPatch * mediaPatch;
    PNotifier        commandNotifier;

    unsigned targetBitRateKbit;
    PINDEX totalLength;
    PTimeInterval newTime;
};

typedef PSafePtr<OpalMediaStream> OpalMediaStreamPtr;


/**This class describes a media stream that is used for media bypass.
  */
class OpalNullMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalNullMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for RTP sessions.
      */
    OpalNullMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                        ///<  Is a source stream
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Read raw media data from the source media stream.
       The default behaviour does nothing and returns false.
      */
    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour does nothing and returns false.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );
	
    /**Indicate if the media stream requires a OpalMediaPatch thread (active patch).
       The default behaviour returns false.
      */
    virtual PBoolean RequiresPatchThread() const;

    /**Indicate if the media stream is synchronous.
       Returns false.
      */
    virtual PBoolean IsSynchronous() const;
  //@}

};


/**This class describes a media stream that transfers data to/from a RTP
   session.
  */
class OpalRTPMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalRTPMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for RTP sessions.
      */
    OpalRTPMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      bool isSource,                       ///<  Is a source stream
      RTP_Session & rtpSession,            ///<  RTP session to stream to/from
      unsigned minAudioJitterDelay,        ///<  Minimum jitter buffer size (if applicable)
      unsigned maxAudioJitterDelay         ///<  Maximum jitter buffer size (if applicable)
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Open the media stream using the media format.

       The default behaviour simply sets the isOpen variable to true.
      */
    virtual PBoolean Open();

    /**Close the media stream.

       The default does nothing.
      */
    virtual PBoolean Close();

    /**Read an RTP frame of data from the source media stream.
       The new behaviour simply calls RTP_Session::ReadData().
      */
    virtual PBoolean ReadPacket(
      RTP_DataFrame & packet
    );

    /**Write an RTP frame of data to the sink media stream.
       The new behaviour simply calls RTP_Session::WriteData().
      */
    virtual PBoolean WritePacket(
      RTP_DataFrame & packet
    );

    /**Set the data size in bytes that is expected to be used.
      */
    virtual PBoolean SetDataSize(
      PINDEX dataSize  ///<  New data size
    );

    /**Indicate if the media stream is synchronous.
       Returns false for RTP streams.
      */
    virtual PBoolean IsSynchronous() const;

    /**Enable jitter buffer for the media stream.

       The default behaviour does nothing.
      */
    virtual void EnableJitterBuffer() const;

    /** Return current RTP session
      */
    virtual RTP_Session & GetRtpSession() const
    { return rtpSession; }

#ifdef OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics) const;
#endif
  //@}

  protected:
    RTP_Session & rtpSession;
    unsigned      minAudioJitterDelay;
    unsigned      maxAudioJitterDelay;
};



/**This class describes a media stream that transfers data to/from a PChannel.
  */
class OpalRawMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalRawMediaStream, OpalMediaStream);
  protected:
  /**@name Construction */
  //@{
    /**Construct a new media stream for channel.
      */
    OpalRawMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      PChannel * channel,                  ///<  I/O channel to stream to/from
      bool autoDelete                      ///<  Automatically delete channel
    );

    /**Delete attached channel if autoDelete enabled.
      */
    ~OpalRawMediaStream();
  //@}

  public:
  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Read raw media data from the source media stream.
       The default behaviour reads from the PChannel object.
      */
    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour writes to the PChannel object.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );

    /**Return the associated PChannel 
     */
    PChannel * GetChannel() { return channel; }
    
    /**Close the media stream.

       Closes the associated PChannel.
      */
    virtual PBoolean Close();

    /**Get average signal level in last frame.
      */
    virtual unsigned GetAverageSignalLevel();
  //@}

  protected:
    PChannel * channel;
    bool       autoDelete;

    PUInt64    averageSignalSum;
    unsigned   averageSignalSamples;
    PMutex     averagingMutex;
    void CollectAverage(const BYTE * buffer, PINDEX size);
};



/**This class describes a media stream that transfers data to/from a file.
  */
class OpalFileMediaStream : public OpalRawMediaStream
{
    PCLASSINFO(OpalFileMediaStream, OpalRawMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for files.
      */
    OpalFileMediaStream(
      OpalConnection &,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      PFile * file,                        ///<  File to stream to/from
      bool autoDelete = true               ///<  Automatically delete file
    );

    /**Construct a new media stream for files.
      */
    OpalFileMediaStream(
      OpalConnection & ,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      const PFilePath & path               ///<  File path to stream to/from
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Indicate if the media stream is synchronous.
       Returns true for LID streams.
      */
    virtual PBoolean IsSynchronous() const;
  //@}

    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour writes to the PChannel object.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );

  protected:
    PFile file;
    PAdaptiveDelay fileDelay;
};

#if OPAL_AUDIO
#if P_AUDIO

/**This class describes a media stream that transfers data to/from a audio
   PSoundChannel.
  */
class PSoundChannel;

class OpalAudioMediaStream : public OpalRawMediaStream
{
    PCLASSINFO(OpalAudioMediaStream, OpalRawMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for audio.
      */
    OpalAudioMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      PINDEX buffers,                      ///<  Number of buffers on sound channel
      PSoundChannel * channel,             ///<  Audio device to stream to/from
      bool autoDelete = true               ///<  Automatically delete PSoundChannel
    );

    /**Construct a new media stream for audio.
      */
    OpalAudioMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      PINDEX buffers,                      ///<  Number of buffers on sound channel
      const PString & deviceName           ///<  Name of audio device to stream to/from
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Set the data size in bytes that is expected to be used. Some media
       streams can make use of this information to perform optimisations.

       The defafault simply sets teh member variable defaultDataSize.
      */
    virtual PBoolean SetDataSize(
      PINDEX dataSize  ///<  New data size
    );

    /**Indicate if the media stream is synchronous.
       Returns true for LID streams.
      */
    virtual PBoolean IsSynchronous() const;
  //@}

  protected:
    PINDEX soundChannelBuffers;
};

#endif

#endif // OPAL_AUDIO

#if OPAL_VIDEO

/**This class describes a media stream that transfers data to/from a
   PVideoDevice.
  */
class PVideoInputDevice;
class PVideoOutputDevice;

class OpalVideoMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalVideoMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for channel.
      */
    OpalVideoMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      PVideoInputDevice * inputDevice,     ///<  Device to use for video grabbing
      PVideoOutputDevice * outputDevice,   ///<  Device to use for video display
      bool autoDelete = true               ///<  Automatically delete PVideoDevices
    );

    /**Delete attached channel if autoDelete enabled.
      */
    ~OpalVideoMediaStream();
  //@}

  /**@name Overrides of PChannel class */
  //@{
    /**Open the media stream.

       The default behaviour sets the OpalLineInterfaceDevice format and
       calls Resume() on the associated OpalMediaPatch thread.
      */
    virtual PBoolean Open();

    /**Close the media stream.

       The default does nothing.
      */
    virtual PBoolean Close();

    /**Read raw media data from the source media stream.
       The default behaviour simply calls ReadPacket() on the data portion of the
       RTP_DataFrame and sets the frames timestamp and marker from the internal
       member variables of the media stream class.
      */
    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour calls WritePacket() on the data portion of the
       RTP_DataFrame and and sets the internal timestamp and marker from the
       member variables of the media stream class.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );

    /**Indicate if the media stream is synchronous.
       Returns true for LID streams.
      */
    virtual PBoolean IsSynchronous() const;

    /** Override size of frame header is included
      */
    virtual PBoolean SetDataSize(
     PINDEX dataSize  ///<  New data size
    );

    /** Get the input device (e.g. for statistics)
      */
    virtual PVideoInputDevice * GetVideoInputDevice() const {
      return inputDevice;
    }

    /** Get the output device (e.g. for statistics)
      */
    virtual PVideoOutputDevice * GetVideoOutputDevice() const {
      return outputDevice;
    }

  //@}

  protected:
    PVideoInputDevice  * inputDevice;
    PVideoOutputDevice * outputDevice;
    bool                 autoDelete;
    PTimeInterval        lastGrabTime;
};

#endif // OPAL_VIDEO

class OpalTransportUDP;

/** Media stream that uses UDP.
 */
class OpalUDPMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalUDPMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for channel.
      */
    OpalUDPMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      OpalTransportUDP & transport         ///<  UDP transport instance
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{

    /**Read an RTP frame of data from the source media stream.
       The new behaviour simply calls OpalTransportUDP::ReadPDU().
      */
    virtual PBoolean ReadPacket(
      RTP_DataFrame & packet
    );

    /**Write an RTP frame of data to the sink media stream.
       The new behaviour simply calls OpalTransportUDP::Write().
      */
    virtual PBoolean WritePacket(
      RTP_DataFrame & packet
    );

    /**Indicate if the media stream is synchronous.
       Returns false.
      */
    virtual PBoolean IsSynchronous() const;

    /**Close the media stream.
       Closes the associated OpalTransportUDP.
      */
    virtual PBoolean Close();

  //@}

  private:
    OpalTransportUDP & udpTransport;
};

#endif //__OPAL_MEDIASTRM_H


// End of File ///////////////////////////////////////////////////////////////
