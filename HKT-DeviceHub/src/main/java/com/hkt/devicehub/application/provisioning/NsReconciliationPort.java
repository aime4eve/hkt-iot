package com.hkt.devicehub.application.provisioning;

import com.hkt.devicehub.domain.model.RegisteredDevice;

import java.util.List;

/**
 * Optional isolation step for upstream network-server reconciliation (e.g.
 * ChirpStack NS). DeviceHub core does not depend on any NS; projects that
 * need NS-side checks provide their own implementation. The default bean is
 * a no-op.
 */
public interface NsReconciliationPort {

    /** Called after a device is registered/bound; may throw to veto the transition. */
    default void afterRegister(RegisteredDevice device) {
    }

    /** Extra difference codes contributed to the reconcile report. */
    default List<String> reconcileDifferences(RegisteredDevice device) {
        return List.of();
    }
}
