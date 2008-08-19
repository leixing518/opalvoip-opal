/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.35
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package -I/home/robertj/opal/include;

public final class OpalCallEndReason {
  public final static OpalCallEndReason OpalCallEndedByLocalUser = new OpalCallEndReason("OpalCallEndedByLocalUser");
  public final static OpalCallEndReason OpalCallEndedByNoAccept = new OpalCallEndReason("OpalCallEndedByNoAccept");
  public final static OpalCallEndReason OpalCallEndedByAnswerDenied = new OpalCallEndReason("OpalCallEndedByAnswerDenied");
  public final static OpalCallEndReason OpalCallEndedByRemoteUser = new OpalCallEndReason("OpalCallEndedByRemoteUser");
  public final static OpalCallEndReason OpalCallEndedByRefusal = new OpalCallEndReason("OpalCallEndedByRefusal");
  public final static OpalCallEndReason OpalCallEndedByNoAnswer = new OpalCallEndReason("OpalCallEndedByNoAnswer");
  public final static OpalCallEndReason OpalCallEndedByCallerAbort = new OpalCallEndReason("OpalCallEndedByCallerAbort");
  public final static OpalCallEndReason OpalCallEndedByTransportFail = new OpalCallEndReason("OpalCallEndedByTransportFail");
  public final static OpalCallEndReason OpalCallEndedByConnectFail = new OpalCallEndReason("OpalCallEndedByConnectFail");
  public final static OpalCallEndReason OpalCallEndedByGatekeeper = new OpalCallEndReason("OpalCallEndedByGatekeeper");
  public final static OpalCallEndReason OpalCallEndedByNoUser = new OpalCallEndReason("OpalCallEndedByNoUser");
  public final static OpalCallEndReason OpalCallEndedByNoBandwidth = new OpalCallEndReason("OpalCallEndedByNoBandwidth");
  public final static OpalCallEndReason OpalCallEndedByCapabilityExchange = new OpalCallEndReason("OpalCallEndedByCapabilityExchange");
  public final static OpalCallEndReason OpalCallEndedByCallForwarded = new OpalCallEndReason("OpalCallEndedByCallForwarded");
  public final static OpalCallEndReason OpalCallEndedBySecurityDenial = new OpalCallEndReason("OpalCallEndedBySecurityDenial");
  public final static OpalCallEndReason OpalCallEndedByLocalBusy = new OpalCallEndReason("OpalCallEndedByLocalBusy");
  public final static OpalCallEndReason OpalCallEndedByLocalCongestion = new OpalCallEndReason("OpalCallEndedByLocalCongestion");
  public final static OpalCallEndReason OpalCallEndedByRemoteBusy = new OpalCallEndReason("OpalCallEndedByRemoteBusy");
  public final static OpalCallEndReason OpalCallEndedByRemoteCongestion = new OpalCallEndReason("OpalCallEndedByRemoteCongestion");
  public final static OpalCallEndReason OpalCallEndedByUnreachable = new OpalCallEndReason("OpalCallEndedByUnreachable");
  public final static OpalCallEndReason OpalCallEndedByNoEndPoint = new OpalCallEndReason("OpalCallEndedByNoEndPoint");
  public final static OpalCallEndReason OpalCallEndedByHostOffline = new OpalCallEndReason("OpalCallEndedByHostOffline");
  public final static OpalCallEndReason OpalCallEndedByTemporaryFailure = new OpalCallEndReason("OpalCallEndedByTemporaryFailure");
  public final static OpalCallEndReason OpalCallEndedByQ931Cause = new OpalCallEndReason("OpalCallEndedByQ931Cause");
  public final static OpalCallEndReason OpalCallEndedByDurationLimit = new OpalCallEndReason("OpalCallEndedByDurationLimit");
  public final static OpalCallEndReason OpalCallEndedByInvalidConferenceID = new OpalCallEndReason("OpalCallEndedByInvalidConferenceID");
  public final static OpalCallEndReason OpalCallEndedByNoDialTone = new OpalCallEndReason("OpalCallEndedByNoDialTone");
  public final static OpalCallEndReason OpalCallEndedByNoRingBackTone = new OpalCallEndReason("OpalCallEndedByNoRingBackTone");
  public final static OpalCallEndReason OpalCallEndedByOutOfService = new OpalCallEndReason("OpalCallEndedByOutOfService");
  public final static OpalCallEndReason OpalCallEndedByAcceptingCallWaiting = new OpalCallEndReason("OpalCallEndedByAcceptingCallWaiting");
  public final static OpalCallEndReason OpalCallEndedWithQ931Code = new OpalCallEndReason("OpalCallEndedWithQ931Code", exampleJNI.OpalCallEndedWithQ931Code_get());

  public final int swigValue() {
    return swigValue;
  }

  public String toString() {
    return swigName;
  }

  public static OpalCallEndReason swigToEnum(int swigValue) {
    if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
      return swigValues[swigValue];
    for (int i = 0; i < swigValues.length; i++)
      if (swigValues[i].swigValue == swigValue)
        return swigValues[i];
    throw new IllegalArgumentException("No enum " + OpalCallEndReason.class + " with value " + swigValue);
  }

  private OpalCallEndReason(String swigName) {
    this.swigName = swigName;
    this.swigValue = swigNext++;
  }

  private OpalCallEndReason(String swigName, int swigValue) {
    this.swigName = swigName;
    this.swigValue = swigValue;
    swigNext = swigValue+1;
  }

  private OpalCallEndReason(String swigName, OpalCallEndReason swigEnum) {
    this.swigName = swigName;
    this.swigValue = swigEnum.swigValue;
    swigNext = this.swigValue+1;
  }

  private static OpalCallEndReason[] swigValues = { OpalCallEndedByLocalUser, OpalCallEndedByNoAccept, OpalCallEndedByAnswerDenied, OpalCallEndedByRemoteUser, OpalCallEndedByRefusal, OpalCallEndedByNoAnswer, OpalCallEndedByCallerAbort, OpalCallEndedByTransportFail, OpalCallEndedByConnectFail, OpalCallEndedByGatekeeper, OpalCallEndedByNoUser, OpalCallEndedByNoBandwidth, OpalCallEndedByCapabilityExchange, OpalCallEndedByCallForwarded, OpalCallEndedBySecurityDenial, OpalCallEndedByLocalBusy, OpalCallEndedByLocalCongestion, OpalCallEndedByRemoteBusy, OpalCallEndedByRemoteCongestion, OpalCallEndedByUnreachable, OpalCallEndedByNoEndPoint, OpalCallEndedByHostOffline, OpalCallEndedByTemporaryFailure, OpalCallEndedByQ931Cause, OpalCallEndedByDurationLimit, OpalCallEndedByInvalidConferenceID, OpalCallEndedByNoDialTone, OpalCallEndedByNoRingBackTone, OpalCallEndedByOutOfService, OpalCallEndedByAcceptingCallWaiting, OpalCallEndedWithQ931Code };
  private static int swigNext = 0;
  private final int swigValue;
  private final String swigName;
}

