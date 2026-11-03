package com.hkt.devicehub.domain.repository;

import com.hkt.devicehub.domain.model.NsCounterSnapshot;
import org.springframework.data.jpa.repository.JpaRepository;

import java.time.Instant;
import java.util.List;

public interface NsCounterSnapshotRepository extends JpaRepository<NsCounterSnapshot, Long> {

    List<NsCounterSnapshot> findByPolledAtGreaterThanEqualOrderByPolledAtAsc(Instant since);
}
