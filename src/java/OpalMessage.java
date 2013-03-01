/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.9
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip.opal;

public class OpalMessage {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected OpalMessage(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OpalMessage obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        OPALJNI.delete_OpalMessage(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setM_type(OpalMessageType value) {
    OPALJNI.OpalMessage_m_type_set(swigCPtr, value.swigValue());
  }

  public OpalMessageType getM_type() {
    return OpalMessageType.swigToEnum(OPALJNI.OpalMessage_m_type_get(swigCPtr));
  }

  public void setM_param(OpalMessageParam value) {
    OPALJNI.OpalMessage_m_param_set(swigCPtr, OpalMessageParam.getCPtr(value), value);
  }

  public OpalMessageParam getM_param() {
    long cPtr = OPALJNI.OpalMessage_m_param_get(swigCPtr);
    return (cPtr == 0) ? null : new OpalMessageParam(cPtr, false);
  }

  public OpalMessage() {
    this(OPALJNI.new_OpalMessage(), true);
  }

}
