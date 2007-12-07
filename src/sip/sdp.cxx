/*
 * sdp.cxx
 *
 * Session Description Protocol support.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2000 Equivalence Pty. Ltd.
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

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "sdp.h"
#endif

#include <sip/sdp.h>

#include <ptlib/socket.h>
#include <opal/transports.h>


#define  SIP_DEFAULT_SESSION_NAME  "Opal SIP Session"
#define  SDP_MEDIA_TRANSPORT       "RTP/AVP"

#define new PNEW


/////////////////////////////////////////////////////////

static OpalTransportAddress ParseConnectAddress(const PStringArray & tokens, PINDEX offset)
{
  if (tokens.GetSize() == offset+3) {
    if (tokens[offset] *= "IN") {
      if (
        (tokens[offset+1] *= "IP4")
#if P_HAS_IPV6
        || (tokens[offset+1] *= "IP6")
#endif
        )
        return OpalTransportAddress(tokens[offset+2], 0, "udp");
      else
      {
        PTRACE(1, "SDP\tConnect address has invalid address type \"" << tokens[offset+1] << '"');
      }
    }
    else {
      PTRACE(1, "SDP\tConnect address has invalid network \"" << tokens[offset] << '"');
    }
  }
  else {
    PTRACE(1, "SDP\tConnect address has invalid (" << tokens.GetSize() << ") elements");
  }

  return OpalTransportAddress();
}


static OpalTransportAddress ParseConnectAddress(const PString & str)
{
  PStringArray tokens = str.Tokenise(' ');
  return ParseConnectAddress(tokens, 0);
}


static PString GetConnectAddressString(const OpalTransportAddress & address)
{
  PStringStream str;

  PIPSocket::Address ip;
  if (address != 0 && address.GetIpAddress(ip))
    str << "IN IP" << ip.GetVersion() << ' ' << ip;
  else
    str << "IN IP4 0.0.0.0";

  return str;
}


/////////////////////////////////////////////////////////

SDPMediaFormat::SDPMediaFormat(RTP_DataFrame::PayloadTypes pt, const char * _name)
  : payloadType(pt)
  , clockRate(0)
  , encodingName(_name)
  , nteSet(PTrue)
#if OPAL_T38FAX
  , nseSet(PTrue)
#endif
{
  if (encodingName == OpalRFC2833.GetEncodingName())
    AddNTEString("0-15,32-49");
#if OPAL_T38FAX
  else if (encodingName == OpalCiscoNSE.GetEncodingName())
    AddNSEString("192,193");
#endif

  GetMediaFormat(); // Initialise, if possible
}


SDPMediaFormat::SDPMediaFormat(const OpalMediaFormat & fmt,
                               RTP_DataFrame::PayloadTypes pt,
                               const char * nxeString)
  : mediaFormat(fmt)
  , payloadType(pt)
  , clockRate(fmt.GetClockRate())
  , encodingName(fmt.GetEncodingName())
  , nteSet(PTrue)
#if OPAL_T38FAX
  , nseSet(PTrue)
#endif
{
  if (nxeString != NULL) {
#if OPAL_T38FAX
    if (encodingName *= "nse")
      AddNSEString(nxeString);
    else
#endif
      AddNTEString(nxeString);
  }
}


void SDPMediaFormat::SetFMTP(const PString & str)
{
  if (str.IsEmpty())
    return;

  if (encodingName == OpalRFC2833.GetEncodingName()) {
    nteSet.RemoveAll();
    AddNTEString(str);
    return;
  }

#if OPAL_T38FAX
  else if (encodingName == OpalCiscoNSE.GetEncodingName()) {
    nseSet.RemoveAll();
    AddNSEString(str);
    return;
  }
#endif

  fmtp = str;
  if (GetMediaFormat().IsEmpty()) // Use GetMediaFormat() to force creation of member
    return;

  // Fill in the default values for (possibly) missing FMTP options
  PINDEX i;
  for (i = 0; i < mediaFormat.GetOptionCount(); i++) {
    OpalMediaOption & option = const_cast<OpalMediaOption &>(mediaFormat.GetOption(i));
    if (!option.GetFMTPName().IsEmpty() && !option.GetFMTPDefault().IsEmpty())
      option.FromString(option.GetFMTPDefault());
  }

  // See if standard format OPT=VAL;OPT=VAL
  if (str.FindOneOf(";=") == P_MAX_INDEX) {
    // Nope, just save the whole string as is
    mediaFormat.SetOptionString("FMTP", str);
    return;
  }

  // Parse the string for option names and values OPT=VAL;OPT=VAL
  PINDEX sep1prev = 0;
  do {
    PINDEX sep1next = str.Find(';', sep1prev);
    if (sep1next == P_MAX_INDEX)
      sep1next--; // Implicit assumption string is not a couple of gigabytes long ...

    PINDEX sep2pos = str.Find('=', sep1prev);
    if (sep2pos > sep1next)
      sep2pos = sep1next;

    PCaselessString key = str(sep1prev, sep2pos-1).Trim();
    if (key.IsEmpty()) {
      PTRACE(2, "SDP\tBadly formed FMTP parameter \"" << str << '"');
      break;
    }

    OpalMediaOption * option = mediaFormat.FindOption(key);
    if (option == NULL || key != option->GetFMTPName()) {
      for (i = 0; i < mediaFormat.GetOptionCount(); i++) {
        if (key == mediaFormat.GetOption(i).GetFMTPName()) {
          option = const_cast<OpalMediaOption *>(&mediaFormat.GetOption(i));
          break;
        }
      }
    }
    if (option != NULL) {
      PString value = str(sep2pos+1, sep1next-1).Trim();
      if (value.IsEmpty())
        value = "1"; // Assume it is a boolean
      if (!option->FromString(value)) {
        PTRACE(2, "SDP\tCould not set FMTP parameter \"" << key << "\" to value \"" << value << '"');
      }
    }

    sep1prev = sep1next+1;
  } while (sep1prev != P_MAX_INDEX);
}


PString SDPMediaFormat::GetFMTP() const
{
  if (encodingName == OpalRFC2833.GetEncodingName())
    return GetNTEString();

#if OPAL_T38FAX
  if (encodingName == OpalCiscoNSE.GetEncodingName())
    return GetNSEString();
#endif

  if (GetMediaFormat().IsEmpty()) // Use GetMediaFormat() to force creation of member
    return fmtp;

  PString str = mediaFormat.GetOptionString("FMTP");
  if (!str.IsEmpty())
    return str;

  for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); i++) {
    const OpalMediaOption & option = mediaFormat.GetOption(i);
    const PString & name = option.GetFMTPName();
    if (!name.IsEmpty()) {
      PString value = option.AsString();
      if (value != option.GetFMTPDefault()) {
        if (!str.IsEmpty())
          str += ';';
        str += name + '=' + value;
      }
    }
  }

  return !str ? str : fmtp;
}


PString SDPMediaFormat::GetNTEString() const
{
  return GetNXEString(nteSet);
}

void SDPMediaFormat::AddNTEString(const PString & str)
{
  AddNXEString(nteSet, str);
}

void SDPMediaFormat::AddNTEToken(const PString & ostr)
{
  AddNXEToken(nteSet, ostr);
}

#if OPAL_T38FAX

PString SDPMediaFormat::GetNSEString() const
{
  return GetNXEString(nseSet);
}

void SDPMediaFormat::AddNSEString(const PString & str)
{
  AddNXEString(nseSet, str);
}

void SDPMediaFormat::AddNSEToken(const PString & ostr)
{
  AddNXEToken(nseSet, ostr);
}

#endif

PString SDPMediaFormat::GetNXEString(POrdinalSet & nxeSet) const
{
  PString str;
  PINDEX i = 0;
  while (i < 255) {
    if (!nxeSet.Contains(POrdinalKey(i)))
      i++;
    else {
      PINDEX start = i++;
      while (nxeSet.Contains(POrdinalKey(i)))
        i++;
      if (!str.IsEmpty())
        str += ",";
      str += PString(PString::Unsigned, start);
      if (i > start+1)
        str += PString('-') + PString(PString::Unsigned, i-1);
    }
  }

  return str;
}


void SDPMediaFormat::AddNXEString(POrdinalSet & nxeSet, const PString & str)
{
  PStringArray tokens = str.Tokenise(",", PFalse);
  PINDEX i;
  for (i = 0; i < tokens.GetSize(); i++)
    AddNXEToken(nxeSet, tokens[i]);
}


void SDPMediaFormat::AddNXEToken(POrdinalSet & nxeSet, const PString & ostr)
{
  PString str = ostr.Trim();
  if (str[0] == ',')
    str = str.Mid(1);
  if (str.Right(1) == ",")
    str = str.Left(str.GetLength()-1);
  PINDEX pos = str.Find('-');
  if (pos == P_MAX_INDEX)
    nxeSet.Include(new POrdinalKey(str.AsInteger()));
  else {
    PINDEX from = str.Left(pos).AsInteger();
    PINDEX to   = str.Mid(pos+1).AsInteger();
    while (from <= to)
      nxeSet.Include(new POrdinalKey(from++));
  }
}


void SDPMediaFormat::PrintOn(ostream & strm) const
{
  PAssert(!encodingName.IsEmpty(), "SDPAudioMediaFormat encoding name is empty");

  strm << "a=rtpmap:" << (int)payloadType << ' ' << encodingName << '/' << clockRate;
  if (!parameters.IsEmpty())
    strm << '/' << parameters;
  strm << "\r\n";

  PString fmtpString = GetFMTP();
  if (!fmtpString.IsEmpty())
    strm << "a=fmtp:" << (int)payloadType << ' ' << fmtpString << "\r\n";
}


const OpalMediaFormat & SDPMediaFormat::GetMediaFormat() const
{
  if (mediaFormat.IsEmpty()) {
    mediaFormat = OpalMediaFormat(payloadType, clockRate, encodingName, "sip");
  }
  return mediaFormat;
}


void SDPMediaFormat::SetPacketTime(const PString & optionName, unsigned ptime)
{
  if (mediaFormat.HasOption(optionName)) {
    unsigned frameTime = mediaFormat.GetFrameTime();
    unsigned newCount = (ptime*mediaFormat.GetTimeUnits()+frameTime-1)/frameTime;
    mediaFormat.SetOptionInteger(optionName, newCount);
    PTRACE(4, "SDP\tMedia format \"" << mediaFormat << "\" option \"" << optionName
           << "\" set to " << newCount << " packets from " << ptime << " milliseconds");
  }
}


PBoolean SDPMediaDescription::SetTransportAddress(const OpalTransportAddress &t)
{
  PIPSocket::Address ip;
  WORD port = 0;
  if (transportAddress.GetIpAndPort(ip, port)) {
    transportAddress = OpalTransportAddress(t, port);
    return PTrue;
  }
  return PFalse;
}

//////////////////////////////////////////////////////////////////////////////

#if 0
#if PTRACING
ostream & operator<<(ostream & out, SDPMediaDescription::MediaType type)
{
  static const char * const MediaTypeNames[SDPMediaDescription::NumMediaTypes] = {
    "Audio", "Video", "Application", "Image", "Unknown"
  };

  if (type < PARRAYSIZE(MediaTypeNames) && MediaTypeNames[type] != NULL)
    out << MediaTypeNames[type];
  else
    out << "MediaTypes<" << (int)type << '>';

  return out;
}
#endif
#endif

SDPMediaDescription::SDPMediaDescription(const OpalTransportAddress & address, const OpalMediaType & _mediaType, const PCaselessString & _transport)
  : transportAddress(address),
    mediaType(_mediaType),
    transport(_transport)
{
  direction     = Undefined;
  port          = 0;
  mediaTypeDefn = OpalMediaTypeFactory::CreateInstance(mediaType);
}


PBoolean SDPMediaDescription::Decode(const PString & str)
{
  PStringArray tokens = str.Tokenise(" ");

  // ensure there are enough elements on the line
  if (tokens.GetSize() < 4) {
    PTRACE(1, "SDP\tMedia session has only " << tokens.GetSize() << " elements");
    return PFalse;
  }

  // get the media type
  if (tokens[0] != mediaType.c_str()) {
    PTRACE(1, "SDP\tdecoded media type " << tokens[0] << " does not match internal type " << mediaType);
    return PFalse;
  }

  // parse the port and port count
  PString portStr  = tokens[1];
  PINDEX pos = portStr.Find('/');
  if (pos == P_MAX_INDEX) 
    portCount = 1;
  else {
    PTRACE(3, "SDP\tMedia header contains port count - " << portStr);
    portCount = (WORD)portStr.Mid(pos+1).AsUnsigned();
    portStr   = portStr.Left(pos);
  }
  port = (WORD)portStr.AsUnsigned();

  // get the transport
  transport        = tokens[2];

  if (port == 0) 
    PTRACE(4, "SDP\tIgnoring media session " << mediaType << " with port=0");
  else {
    PTRACE(4, "SDP\tMedia session port=" << port);

    PIPSocket::Address ip;
    transportAddress.GetIpAddress(ip);
    transportAddress = OpalTransportAddress(ip, (WORD)port);

    // create the format list
    PINDEX i;
    for (i = 3; i < tokens.GetSize(); i++) 
      formats.Append(CreateSDPMediaFormatFromName(tokens[i]));
  }

  return PTrue;
}


void SDPMediaDescription::SetAttribute(const PString & attr, const PString & value)
{
  // get the attribute type
  if (attr *= "sendonly") {
    direction = SendOnly;
    return;
  }

  if (attr *= "recvonly") {
    direction = RecvOnly;
    return;
  }

  if (attr *= "sendrecv") {
    direction = SendRecv;
    return;
  }

  if (attr *= "inactive") {
    direction = Inactive;
    return;
  }

  // handle rtpmap attribute
  if (attr *= "rtpmap") {
    PString params = value;
    SDPMediaFormat * format = FindFormat(params);
    if (format != NULL) {
      PStringArray tokens = params.Tokenise('/');
      if (tokens.GetSize() < 2) {
        PTRACE(2, "SDP\tMalformed rtpmap attribute for " << format->GetEncodingName());
        return;
      }

      format->SetEncodingName(tokens[0]);
      format->SetClockRate(tokens[1].AsUnsigned());
      if (tokens.GetSize() > 2)
        format->SetParameters(tokens[2]);
    }
    return;
  }

  // handle fmtp attributes
  if (attr *= "fmtp") {
    PString params = value;
    SDPMediaFormat * format = FindFormat(params);
    if (format != NULL)
      format->SetFMTP(params);
    return;
  }

#if OPAL_AUDIO
  if (attr *= "ptime") {
    SetPacketTime(OpalAudioFormat::TxFramesPerPacketOption(), value);
    return;
  }

  if (attr *= "maxptime") {
    SetPacketTime(OpalAudioFormat::RxFramesPerPacketOption(), value);
    return;
  }
#endif

  // unknown attriutes
  PTRACE(2, "SDP\tUnknown media attribute " << attr);
  return;
}


SDPMediaFormat * SDPMediaDescription::FindFormat(PString & str) const
{
  // extract the RTP payload type
  PINDEX pos = str.FindSpan("0123456789");
  if ((pos != P_MAX_INDEX) && !isspace(str[pos])) {
    PTRACE(2, "SDP\tMalformed media attribute requiring format " << str);
    return NULL;
  }

  RTP_DataFrame::PayloadTypes pt = (RTP_DataFrame::PayloadTypes)str.Left(pos).AsUnsigned();

  // extract the attribute argument
  if (pos != P_MAX_INDEX) {
    while (isspace(str[pos]))
      pos++;
    str.Delete(0, pos);
  }

  // find the format that matches the payload type
  for (PINDEX fmt = 0; fmt < formats.GetSize(); fmt++) {
    if (formats[fmt].GetPayloadType() == pt)
      return &formats[fmt];
  }

  PTRACE(2, "SDP\tMedia attribute found for unknown RTP type " << pt);
  return NULL;
}


void SDPMediaDescription::SetPacketTime(const PString & optionName, const PString & value)
{
  unsigned newTime = value.AsUnsigned();
  if (newTime < 10) {
    PTRACE(2, "SDP\tMalformed (max)ptime attribute value " << value);
    return;
  }

  for (PINDEX i = 0; i < formats.GetSize(); i++)
   formats[i].SetPacketTime(optionName, newTime);
}


void SDPMediaDescription::PrintOn(const OpalTransportAddress & commonAddr, ostream & str) const
{
  PIPSocket::Address commonIP;
  commonAddr.GetIpAddress(commonIP);

  PIPSocket::Address transportIP;
  transportAddress.GetIpAddress(transportIP);

  PString connectString;
  if (commonIP != transportIP)
    connectString = GetConnectAddressString(transportAddress);

  PrintOn(str, connectString);
}

void SDPMediaDescription::PrintOn(ostream & str) const
{
  PIPSocket::Address ip;
  transportAddress.GetIpAddress(ip);
  PrintOn(str, GetConnectAddressString(transportAddress));
}

void SDPMediaDescription::PrintOn(ostream & str, const PString & connectString) const
{
  //
  // if no media formats, then do not output the media header
  // this avoids displaying an empty media header with no payload types
  // when (for example) video has been disabled
  //
  if (formats.GetSize() == 0)
    return;

  PIPSocket::Address ip;
  WORD port;
  transportAddress.GetIpAndPort(ip, port);

  // output media header
  str << "m=" 
      << mediaType << " "
      << port << " "
      << transport;

  if (!PrintFormat(str)) {
    PINDEX i;
    for (i = 0; i < formats.GetSize(); i++)
      str << ' ' << formats[i].GetEncodingName();
    str << "\r\n";
  }

  if (!connectString.IsEmpty())
    str << "c=" << connectString << "\r\n";
}

bool SDPMediaDescription::PrintFormat(ostream &) const
{ 
  return false; 
}


OpalMediaFormatList SDPMediaDescription::GetMediaFormats(const OpalMediaType & mediaType) const
{
  OpalMediaFormatList list;

  PINDEX i;
  for (i = 0; i < formats.GetSize(); i++) {
    OpalMediaFormat opalFormat = formats[i].GetMediaFormat();
    if (opalFormat.IsEmpty())
      PTRACE(2, "SIP\tRTP payload type " << formats[i].GetPayloadType() << " not matched to audio codec");
    else {
      if (opalFormat.GetMediaType() == mediaType && 
          opalFormat.IsValidForProtocol("sip") &&
          opalFormat.GetEncodingName() != NULL) {
        PTRACE(3, "SIP\tRTP payload type " << formats[i].GetPayloadType() << " matched to codec " << opalFormat);
        list += opalFormat;
      }
    }
  }

  return list;
}

void SDPMediaDescription::CreateRTPMap(const OpalMediaSessionId & sessionID, RTP_DataFrame::PayloadMapType & map) const
{
  PINDEX i;
  for (i = 0; i < formats.GetSize(); i++) {
    OpalMediaFormat opalFormat = formats[i].GetMediaFormat();
    if (!opalFormat.IsEmpty() && 
         opalFormat.GetMediaType() == sessionID.mediaType &&
         opalFormat.GetPayloadType() != formats[i].GetPayloadType()) {
      map.insert(RTP_DataFrame::PayloadMapType::value_type(opalFormat.GetPayloadType(), formats[i].GetPayloadType()));
      PTRACE(3, "SDP\tAdding RTP translation from " << opalFormat.GetPayloadType() << " to " << formats[i].GetPayloadType());
    }
  }
}

void SDPMediaDescription::AddSDPMediaFormat(SDPMediaFormat * sdpMediaFormat)
{
  formats.Append(sdpMediaFormat);
}

void SDPMediaDescription::AddSDPMediaFormat(const OpalMediaFormat & mediaFormat, RTP_DataFrame::PayloadTypes pt, const char * nteString)
{
  if (mediaTypeDefn == NULL) {
    PTRACE(3, "SDP\tCannot add media format for unknown type " << mediaType);
    return;
  }

  if (mediaType != mediaFormat.GetMediaType()) {
    PTRACE(3, "SDP\tCannot add " << mediaFormat << " to SDP description of type " << mediaType);
    return;
  }

  SDPMediaFormat * sdpFormat = CreateSDPMediaFormatFromFormat(mediaFormat, pt, nteString);
  if (sdpFormat == NULL) {
    PTRACE(3, "SDP\tCannot create SDP format for " << mediaFormat);
    return;
  }

  AddSDPMediaFormat(sdpFormat);
}

void SDPMediaDescription::AddMediaFormat(const OpalMediaFormat & mediaFormat, const RTP_DataFrame::PayloadMapType & map)
{
  if (!mediaFormat.IsTransportable() || !mediaFormat.IsValidForProtocol("sip"))
    return;

  RTP_DataFrame::PayloadTypes payloadType = mediaFormat.GetPayloadType();
  if (map.size() != 0) {
    RTP_DataFrame::PayloadMapType::const_iterator r = map.find(payloadType);
    if (r != map.end())
      payloadType = r->second;
  }

  unsigned clockRate = mediaFormat.GetClockRate();

  for (PINDEX i = 0; i < formats.GetSize(); i++) {
    if (formats[i].GetPayloadType() == payloadType ||
        ((formats[i].GetEncodingName() *= mediaFormat.GetEncodingName()) && formats[i].GetClockRate() == clockRate)
        )
      return;
  }

  SDPMediaFormat * sdpFormat = new SDPMediaFormat(mediaFormat, payloadType);

  AddSDPMediaFormat(sdpFormat);
}


void SDPMediaDescription::AddMediaFormats(const OpalMediaFormatList & mediaFormats, const OpalMediaSessionId & sessionId, const RTP_DataFrame::PayloadMapType & map)
{
  for (PINDEX i = 0; i < mediaFormats.GetSize(); i++) {
    OpalMediaFormat & mediaFormat = mediaFormats[i];
    if (mediaFormat.GetMediaType() == sessionId.mediaType && (sessionId.mediaType == "image" || mediaFormat.IsTransportable()))
      AddMediaFormat(mediaFormat, map);
  }
}


//////////////////////////////////////////////////////////////////////////////

const PString & SDPSessionDescription::ConferenceTotalBandwidthModifier()     { static PString s = "CT"; return s; }
const PString & SDPSessionDescription::ApplicationSpecificBandwidthModifier() { static PString s = "AS"; return s; }

SDPSessionDescription::SDPSessionDescription(const OpalTransportAddress & address)
  : sessionName(SIP_DEFAULT_SESSION_NAME),
    ownerUsername('-'),
    ownerAddress(address),
    defaultConnectAddress(address)
{
  protocolVersion  = 0;
  ownerSessionId  = ownerVersion = (unsigned)PTime().GetTimeInSeconds();
  direction = SDPMediaDescription::Undefined;
  
  bandwidthModifier = "";
  bandwidthValue = 0;
  defaultConnectPort = 0;
}


void SDPSessionDescription::PrintOn(ostream & str) const
{
  OpalTransportAddress connectionAddress(defaultConnectAddress);
  PBoolean useCommonConnect = PTrue;

  // see common connect address is needed
  {
    OpalTransportAddress descrAddress;
    PINDEX matched = 0;
    PINDEX descrMatched = 0;
    PINDEX i;
    for (i = 0; i < mediaDescriptions.GetSize(); i++) {
      if (i == 0)
        descrAddress = mediaDescriptions[i].GetTransportAddress();
      if (mediaDescriptions[i].GetTransportAddress() == connectionAddress)
        ++matched;
      if (mediaDescriptions[i].GetTransportAddress() == descrAddress)
        ++descrMatched;
    }
    if (connectionAddress != descrAddress) {
      if ((descrMatched > matched))
        connectionAddress = descrAddress;
      else
        useCommonConnect = PFalse;
    }
  }

  // encode mandatory session information
  str << "v=" << protocolVersion << "\r\n"
         "o=" << ownerUsername << ' '
        << ownerSessionId << ' '
        << ownerVersion << ' '
              << GetConnectAddressString(ownerAddress)
              << "\r\n"
         "s=" << sessionName << "\r\n";

  // make sure the "c=" line (if required) is before the "t=" otherwise the proxy
  // used by FWD will reject the call with an SDP parse error. This does not seem to be 
  // required by the RFC so it is probably a bug in the proxy
  if (useCommonConnect)
    str << "c=" << GetConnectAddressString(connectionAddress) << "\r\n";
  
  if(bandwidthModifier != "" && bandwidthValue != 0) {
    str << "b=" << bandwidthModifier << ":" << bandwidthValue << "\r\n";
  }
  
  str << "t=" << "0 0" << "\r\n";

  switch (direction) {
    case SDPMediaDescription::RecvOnly:
      str << "a=recvonly" << "\r\n";
      break;
    case SDPMediaDescription::SendOnly:
      str << "a=sendonly" << "\r\n";
      break;
    case SDPMediaDescription::SendRecv:
      str << "a=sendrecv" << "\r\n";
      break;
    case SDPMediaDescription::Inactive:
      str << "a=inactive" << "\r\n";
      break;
    default:
      break;
  }

  // encode media session information
  PINDEX i;
  for (i = 0; i < mediaDescriptions.GetSize(); i++) {
    if (useCommonConnect) 
      mediaDescriptions[i].PrintOn(connectionAddress, str);
    else
      str << mediaDescriptions[i];
  }
}


PString SDPSessionDescription::Encode() const
{
  PStringStream str;
  PrintOn(str);
  return str;
}


PBoolean SDPSessionDescription::Decode(const PString & str)
{
  // break string into lines
  PStringArray lines = str.Lines();

  // parse keyvalue pairs
  SDPMediaDescription * currentMedia = NULL;
  PINDEX i;
  for (i = 0; i < lines.GetSize(); i++) {
    PString & line = lines[i];
    PINDEX pos = line.Find('=');
    if (pos != P_MAX_INDEX) {
      PString key   = line.Left(pos).Trim();
      PString value = line.Mid(pos+1).Trim();
      if (key.GetLength() == 1) {

        // media name and transport address (mandatory)
        if (key[0] == 'm') {
          OpalMediaType mediaType(value.Left(value.Find(' ')));
          OpalMediaTypeDefinition * mediaDef = OpalMediaTypeFactory::CreateInstance(mediaType);
          if (mediaDef == NULL) {
            currentMedia = NULL;
            PTRACE(1, "SDP\tUnknown media type " << mediaType);
          }
          else {
            currentMedia = mediaDef->CreateSDPMediaDescription(mediaType, defaultConnectAddress);
            if (currentMedia == NULL) {
              PTRACE(1, "SDP\tUnable to create media description for media type " << mediaType);
            }
            else {
              if (currentMedia->Decode(value) && (currentMedia->GetPort() != 0)) {
                mediaDescriptions.Append(currentMedia);
                PTRACE(3, "SDP\tAdding media session with " << currentMedia->GetSDPMediaFormats().GetSize() << " formats");
                defaultConnectPort = currentMedia->GetPort();
              }
              else {
                PTRACE(1, "SDP\tUnable to decode media description for media type " << mediaType);
                delete currentMedia;
                currentMedia = NULL;
              }
            }
          }
        }
  
        /////////////////////////////////
        //
        // Session description
        //
        /////////////////////////////////
    
        else if (currentMedia == NULL) {
          PINDEX thePos;
          switch (key[0]) {
            case 'v' : // protocol version (mandatory)
              protocolVersion = value.AsInteger();
              break;

            case 'o' : // owner/creator and session identifier (mandatory)
              ParseOwner(value);
              break;

            case 's' : // session name (mandatory)
              sessionName = value;
              break;

            case 'c' : // connection information - not required if included in all media
              defaultConnectAddress = ParseConnectAddress(value);
              break;

            case 't' : // time the session is active (mandatory)
            case 'i' : // session information
            case 'u' : // URI of description
            case 'e' : // email address
            case 'p' : // phone number
              break;
            case 'b' : // bandwidth information
              thePos = value.Find(':');
              if (thePos != P_MAX_INDEX) {
                bandwidthModifier = value.Left(thePos);
                bandwidthValue = value.Mid(thePos+1).AsInteger();
              }
              break;
            case 'z' : // time zone adjustments
            case 'k' : // encryption key
            case 'r' : // zero or more repeat times
              break;
            case 'a' : // zero or more session attribute lines
              if (value *= "sendonly")
                SetDirection (SDPMediaDescription::SendOnly);
              else if (value *= "recvonly")
                SetDirection (SDPMediaDescription::RecvOnly);
              else if (value *= "sendrecv")
                SetDirection (SDPMediaDescription::SendRecv);
              else if (value *= "inactive")
                SetDirection (SDPMediaDescription::Inactive);
              break;

            default:
              PTRACE(1, "SDP\tUnknown session information key " << key[0]);
          }
        }

        /////////////////////////////////
        //
        // media information
        //
        /////////////////////////////////
    
        else {
          switch (key[0]) {
            case 'i' : // media title
            case 'b' : // bandwidth information
            case 'k' : // encryption key
              break;

            case 'c' : // connection information - optional if included at session-level
              if (defaultConnectPort != 0)
                currentMedia->SetTransportAddress(OpalTransportAddress(ParseConnectAddress(value), defaultConnectPort));
              break;

            case 'a' : // zero or more media attribute lines
              pos = value.FindSpan("!#$%&'*+-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`abcdefghijklmnopqrstuvwxyz{|}~"); // Legal chars from RFC
              if (pos == P_MAX_INDEX)
                currentMedia->SetAttribute(value, "1");
              else if (value[pos] == ':')
                currentMedia->SetAttribute(value.Left(pos), value.Mid(pos+1));
              else {
                PTRACE(2, "SDP\tMalformed media attribute " << value);
              }
              break;

            default:
              PTRACE(1, "SDP\tUnknown media information key " << key[0]);
          }
        }
      }
    }
  }

  return PTrue;
}


void SDPSessionDescription::ParseOwner(const PString & str)
{
  PStringArray tokens = str.Tokenise(" ");

  if (tokens.GetSize() != 6) {
    PTRACE(2, "SDP\tOrigin has incorrect number of elements (" << tokens.GetSize() << ')');
  }
  else {
    ownerUsername    = tokens[0];
    ownerSessionId   = tokens[1].AsUnsigned();
    ownerVersion     = tokens[2].AsUnsigned();
    ownerAddress = defaultConnectAddress = ParseConnectAddress(tokens, 3);
  }
}


SDPMediaDescription * SDPSessionDescription::GetMediaDescription(const std::string & mediaType) const
{
  // look for matching media type
  PINDEX i;
  for (i = 0; i < mediaDescriptions.GetSize(); i++) {
    if (mediaDescriptions[i].GetMediaType() == mediaType)
      return &mediaDescriptions[i];
  }

  return NULL;
}

SDPMediaDescription * SDPSessionDescription::GetMediaDescription(const SessionKey & sessionId) const
{
  // look for matching session ID
  PINDEX i;
  for (i = 0; i < mediaDescriptions.GetSize(); i++) {
    if (mediaDescriptions[i].GetPort() == sessionId)
      return &mediaDescriptions[i];
  }

  return NULL;
}



SDPMediaDescription::Direction SDPSessionDescription::GetDirection(const OpalMediaSessionId & sessionID) const
{
  PINDEX i;
  for (i = 0; i < mediaDescriptions.GetSize(); i++) {
    if ((mediaDescriptions[i].GetMediaType() == "video" && sessionID.sessionId == 2) || 
        (mediaDescriptions[i].GetMediaType() == "audio" && sessionID.sessionId == 1)) {
      if (mediaDescriptions[i].GetDirection() != SDPMediaDescription::Undefined)
        return mediaDescriptions[i].GetDirection();
      else
        return direction;
    }
  }
  
  return direction;
}

//////////////////////////////////////////////////////////////////////////////

OpalRTPAVPSDPMediaDescription::OpalRTPAVPSDPMediaDescription(const OpalMediaType & _mediaType, const OpalTransportAddress & _address)
  : SDPMediaDescription(_address, _mediaType, SDP_MEDIA_TRANSPORT)
{ }

SDPMediaFormat * OpalRTPAVPSDPMediaDescription::CreateSDPMediaFormatFromName(const PString & token)
{ 
  return new SDPMediaFormat((RTP_DataFrame::PayloadTypes)token.AsUnsigned());
}

SDPMediaFormat * OpalRTPAVPSDPMediaDescription::CreateSDPMediaFormatFromFormat(const OpalMediaFormat & mediaFormat, RTP_DataFrame::PayloadTypes pt, const char * nteString)
{
  return new SDPMediaFormat(mediaFormat, pt, nteString) ;
}

void OpalRTPAVPSDPMediaDescription::SetAttribute(const PString & attr, const PString & value)
{
  if (attr *= "ptime") {
    SetPacketTime(OpalAudioFormat::TxFramesPerPacketOption(), value);
    return;
  }

  if (attr *= "maxptime") {
    SetPacketTime(OpalAudioFormat::RxFramesPerPacketOption(), value);
    return;
  }

  SDPMediaDescription::SetAttribute(attr, value);
}

bool OpalRTPAVPSDPMediaDescription::PrintFormat(ostream & str) const
{
  // output RTP payload types
  PINDEX i;
  for (i = 0; i < formats.GetSize(); i++)
    str << ' ' << (int)formats[i].GetPayloadType();
  str << "\r\n";

  // output attributes for each payload type
  for (i = 0; i < formats.GetSize(); i++)
    str << formats[i];

#if OPAL_AUDIO && defined(HAVE_PTIME)
  // Fill in the ptime  as maximum tx packets of all media formats
  // and maxptime as minimum rx packets of all media formats
  unsigned ptime = 0;
  unsigned maxptime = UINT_MAX;

  // output attributes for each payload type
  for (i = 0; i < formats.GetSize(); i++) {
    const OpalMediaFormat & mediaFormat = formats[i].GetMediaFormat();
    if (mediaFormat.HasOption(OpalAudioFormat::TxFramesPerPacketOption())) {
      unsigned ptime1 = txFrames*mediaFormat.GetFrameTime()/mediaFormat.GetTimeUnits();
      if (ptime < ptime1)
        ptime = ptime1;
    }
    if (mediaFormat.HasOption(OpalAudioFormat::RxFramesPerPacketOption())) {
      unsigned maxptime1 = mediaFormat.GetOptionInteger(OpalAudioFormat::RxFramesPerPacketOption())*mediaFormat.GetFrameTime()/mediaFormat.GetTimeUnits();
      if (maxptime > maxptime1)
        maxptime = maxptime1;
    }
  }

  // don't output ptime parameters, as some Cisco endpoints barf on it
  // and it's not very well-defined anyway
  //if (ptime > 0)
  //  str << "a=ptime:" << ptime << "\r\n";

  if (maxptime < UINT_MAX)
    str << "a=maxptime:" << maxptime << "\r\n";
#endif // OPAL_AUDIO

  // media format direction
  switch (direction) {
    case SDPMediaDescription::RecvOnly:
      str << "a=recvonly" << "\r\n";
      break;
    case SDPMediaDescription::SendOnly:
      str << "a=sendonly" << "\r\n";
      break;
    case SDPMediaDescription::SendRecv:
      str << "a=sendrecv" << "\r\n";
      break;
    case SDPMediaDescription::Inactive:
      str << "a=inactive" << "\r\n";
      break;
    default:
      break;
  }

  return true;
}

// End of file ////////////////////////////////////////////////////////////////
