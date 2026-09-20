package com.hkt.devicehub.infrastructure.thingsboard;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.hkt.devicehub.application.LastFrameTracker;
import com.hkt.devicehub.application.TelemetryFrameDispatcher;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.monitoring.TelemetryChannelMetrics;
import jakarta.annotation.PostConstruct;
import jakarta.annotation.PreDestroy;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.stereotype.Component;

import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.WebSocket;
import java.time.Duration;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletionStage;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

/**
 * ThingsBoard WebSocket telemetry channel (primary). Subscribes
 * /api/ws latest values of every ACTIVE registered device for the keys
 * result / dataHex / rssi / snr / downLinkGateway, normalizes each point and
 * dispatches frames to RocketMQ. 30s ping, 1s→60s exponential backoff
 * reconnect (parking TbWebSocketChannel blueprint).
 */
@Component
@ConditionalOnProperty(name = "devicehub.tb.enabled", havingValue = "true")
public class TbWebSocketChannel {

    private static final Logger log = LoggerFactory.getLogger(TbWebSocketChannel.class);
    private static final List<String> SUBSCRIBED_KEYS =
            List.of("result", "dataHex", "rssi", "snr", "downLinkGateway");

    private final TbClient client;
    private final TbProperties props;
    private final TbFrameNormalizer normalizer;
    private final TelemetryFrameDispatcher dispatcher;
    private final LastFrameTracker lastFrameTracker;
    private final RegisteredDeviceRepository devices;
    private final TelemetryChannelMetrics metrics;
    private final ObjectMapper mapper = new ObjectMapper();
    private final HttpClient http = HttpClient.newHttpClient();
    private final ScheduledExecutorService scheduler = Executors.newSingleThreadScheduledExecutor(r -> {
        Thread t = new Thread(r, "tb-ws");
        t.setDaemon(true);
        return t;
    });
    final Map<Integer, RegisteredDevice> subscriptions = new ConcurrentHashMap<>();
    private final StringBuilder buffer = new StringBuilder();
    private final AtomicBoolean reconnectPending = new AtomicBoolean();
    private final AtomicLong epoch = new AtomicLong();
    private volatile WebSocket socket;
    private volatile ScheduledFuture<?> ping;
    private volatile boolean running;
    private long backoff = 1_000;

    public TbWebSocketChannel(TbClient client, TbProperties props, TbFrameNormalizer normalizer,
                              TelemetryFrameDispatcher dispatcher,
                              LastFrameTracker lastFrameTracker,
                              RegisteredDeviceRepository devices,
                              TelemetryChannelMetrics metrics) {
        this.client = client;
        this.props = props;
        this.normalizer = normalizer;
        this.dispatcher = dispatcher;
        this.lastFrameTracker = lastFrameTracker;
        this.devices = devices;
        this.metrics = metrics;
    }

    @PostConstruct
    public synchronized void start() {
        if (!props.isWsEnabled()) {
            log.info("[TbWs] disabled");
            return;
        }
        running = true;
        connect();
    }

    private void connect() {
        if (!running) return;
        metrics.setChannelState(TelemetryChannelMetrics.CHANNEL_WS, false);
        List<RegisteredDevice> active = devices.findByStatus(RegistrationStatus.ACTIVE).stream()
                .filter(d -> d.getTbDeviceId() != null)
                .toList();
        if (active.isEmpty()) {
            log.info("[TbWs] no ACTIVE devices, retry later");
            scheduleReconnect();
            return;
        }
        long currentEpoch = epoch.incrementAndGet();
        http.newWebSocketBuilder().connectTimeout(Duration.ofSeconds(10))
                .buildAsync(URI.create(props.getBaseUrl().replaceFirst("^http", "ws") + "/api/ws"),
                        new Listener(currentEpoch))
                .thenAccept(ws -> {
                    if (currentEpoch != epoch.get()) ws.abort();
                    else onOpen(ws, currentEpoch, active);
                })
                .exceptionally(ex -> {
                    if (currentEpoch == epoch.get()) scheduleReconnect();
                    return null;
                });
    }

    private void onOpen(WebSocket ws, long currentEpoch, List<RegisteredDevice> active) {
        try {
            socket = ws;
            ObjectNode message = mapper.createObjectNode();
            ObjectNode auth = message.putObject("authCmd");
            auth.put("cmdId", 0).put("token", client.getAccessToken());
            ArrayNode cmds = message.putArray("cmds");
            subscriptions.clear();
            int id = 0;
            for (RegisteredDevice device : active) {
                subscriptions.put(++id, device);
                cmds.add(subscription(id, device.getTbDeviceId().toString()));
            }
            ws.sendText(mapper.writeValueAsString(message), true).join();
            backoff = 1_000;
            startPing();
            metrics.setChannelState(TelemetryChannelMetrics.CHANNEL_WS, true);
            log.info("[TbWs] connected with {} subscriptions", subscriptions.size());
        } catch (Exception e) {
            log.warn("[TbWs] subscribe failed: {}", e.getMessage());
            try {
                ws.abort();
            } catch (Exception ignored) {
            }
            if (currentEpoch == epoch.get()) scheduleReconnect();
        }
    }

    public ObjectNode subscription(int cmdId, String deviceId) {
        ObjectNode cmd = mapper.createObjectNode();
        cmd.put("cmdId", cmdId).put("type", "ENTITY_DATA");
        ObjectNode query = cmd.putObject("query");
        ObjectNode filter = query.putObject("entityFilter");
        filter.put("type", "singleEntity");
        filter.putObject("singleEntity").put("entityType", "DEVICE").put("id", deviceId);
        query.putObject("pageLink").put("pageSize", 10).put("page", 0);
        ArrayNode fields = query.putArray("entityFields");
        fields.addObject().put("type", "ENTITY_FIELD").put("key", "name");
        query.putArray("latestValues");
        ObjectNode latest = cmd.putObject("latestCmd");
        ArrayNode keys = latest.putArray("keys");
        for (String key : SUBSCRIBED_KEYS) {
            keys.addObject().put("type", "TIME_SERIES").put("key", key);
        }
        return cmd;
    }

    public void handle(String text) {
        try {
            JsonNode root = mapper.readTree(text);
            JsonNode update = root.path("update");
            if (!update.isArray()) return;
            RegisteredDevice device = subscriptions.get(root.path("cmdId").asInt());
            if (device == null) return;
            for (JsonNode item : update) {
                JsonNode series = item.path("latest").path("TIME_SERIES");
                for (String key : SUBSCRIBED_KEYS) {
                    process(device, key, series.path(key));
                }
            }
        } catch (Exception e) {
            log.warn("[TbWs] message failed: {}", e.getMessage());
        }
    }

    private void process(RegisteredDevice device, String key, JsonNode point) {
        long ts = point.path("ts").asLong();
        JsonNode value = point.path("value");
        if (ts <= 0 || value.isMissingNode() || value.isNull()) return;
        TbFrameNormalizer.DeviceRef ref = new TbFrameNormalizer.DeviceRef(
                device.getTbDeviceId().toString(), device.getDevEui(), device.getProject().name());
        normalizer.normalizeWsPoint(ref, key, ts, value)
                .ifPresent(frame -> {
                    lastFrameTracker.record(device.getId(), frame.ts());
                    dispatcher.dispatch(device, frame, TelemetryChannelMetrics.CHANNEL_WS);
                });
    }

    private synchronized void scheduleReconnect() {
        stopPing();
        if (!running || !reconnectPending.compareAndSet(false, true)) return;
        metrics.setChannelState(TelemetryChannelMetrics.CHANNEL_WS, false);
        metrics.recordReconnect(TelemetryChannelMetrics.CHANNEL_WS);
        long delay = backoff;
        backoff = Math.min(backoff * 2, 60_000);
        scheduler.schedule(() -> {
            reconnectPending.set(false);
            connect();
        }, delay, TimeUnit.MILLISECONDS);
        log.info("[TbWs] reconnect in {}ms", delay);
    }

    private void startPing() {
        stopPing();
        ping = scheduler.scheduleAtFixedRate(() -> {
            try {
                if (socket != null) socket.sendPing(java.nio.ByteBuffer.wrap(new byte[]{1}));
            } catch (Exception ignored) {
            }
        }, props.getWsPingMs(), props.getWsPingMs(), TimeUnit.MILLISECONDS);
    }

    private void stopPing() {
        if (ping != null) {
            ping.cancel(false);
            ping = null;
        }
    }

    @PreDestroy
    public synchronized void stop() {
        running = false;
        stopPing();
        scheduler.shutdownNow();
        metrics.setChannelState(TelemetryChannelMetrics.CHANNEL_WS, false);
        if (socket != null) try {
            socket.abort();
        } catch (Exception ignored) {
        }
    }

    private final class Listener implements WebSocket.Listener {
        private final long currentEpoch;

        private Listener(long currentEpoch) {
            this.currentEpoch = currentEpoch;
        }

        @Override
        public CompletionStage<?> onText(WebSocket ws, CharSequence data, boolean last) {
            if (currentEpoch != epoch.get()) {
                ws.request(1);
                return null;
            }
            buffer.append(data);
            if (last) {
                String message = buffer.toString();
                buffer.setLength(0);
                handle(message);
            }
            ws.request(1);
            return null;
        }

        @Override
        public CompletionStage<?> onClose(WebSocket ws, int status, String reason) {
            if (currentEpoch == epoch.get()) {
                log.warn("[TbWs] closed {}: {}", status, reason);
                scheduleReconnect();
            }
            return null;
        }

        @Override
        public void onError(WebSocket ws, Throwable error) {
            if (currentEpoch == epoch.get()) {
                log.warn("[TbWs] error: {}", error.toString());
                scheduleReconnect();
            }
        }
    }
}
