package com.hkt.devicehub.domain.repository;

import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.List;
import java.util.Optional;

public interface RegisteredDeviceRepository extends JpaRepository<RegisteredDevice, Long> {

    Optional<RegisteredDevice> findByProjectAndDevEui(DeviceProject project, String devEui);

    List<RegisteredDevice> findByProject(DeviceProject project);

    List<RegisteredDevice> findByStatus(RegistrationStatus status);
}
