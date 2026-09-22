package com.hkt.devicehub.domain.repository;

import com.hkt.devicehub.domain.model.AuditLog;
import org.springframework.data.jpa.repository.JpaRepository;

public interface AuditLogRepository extends JpaRepository<AuditLog, Long> {
}
