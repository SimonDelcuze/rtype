# NetworkStatsComponent

## Overview

The `NetworkStatsComponent` is a diagnostic and monitoring component in the R-Type client that tracks network performance metrics including ping, jitter, packet loss, and bandwidth usage. It provides essential data for debugging network issues, implementing adaptive quality settings, and displaying connection quality information to players.

## Purpose and Design Philosophy

Network-aware games require visibility into connection quality:

1. **Latency Monitoring**: Track round-trip time (ping) for responsiveness
2. **Jitter Analysis**: Measure variance in latency for interpolation tuning
3. **Packet Loss Detection**: Identify connection reliability issues
4. **Bandwidth Tracking**: Monitor data usage for optimization
5. **Player Feedback**: Show connection quality indicators in HUD

The component maintains rolling histories for statistical analysis while keeping memory usage bounded through fixed-size buffers.

## Component Structure

```cpp
struct NetworkStatsComponent
{
    float currentPing             = 0.0F;
    float averagePing             = 0.0F;
    float minPing                 = 999.0F;
    float maxPing                 = 0.0F;
    float jitter                  = 0.0F;
    std::uint32_t packetsSent     = 0;
    std::uint32_t packetsReceived = 0;
    std::uint32_t packetsLost     = 0;
    float packetLossRate          = 0.0F;
    float bandwidthIn             = 0.0F;
    float bandwidthOut            = 0.0F;
    float lastUpdateTime          = 0.0F;
    float timeSinceUpdate         = 0.0F;
    std::deque<float> pingHistory;
    std::deque<std::uint32_t> bytesInHistory;
    std::deque<std::uint32_t> bytesOutHistory;
    static constexpr std::size_t kMaxHistorySize = 60;

    static NetworkStatsComponent create();
    void addPingSample(float ping);
    void addBandwidthSample(std::uint32_t bytesIn, std::uint32_t bytesOut);
    void updatePacketLoss();
};
```

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `currentPing` | `float` | `0.0F` | Most recent ping measurement in milliseconds. |
| `averagePing` | `float` | `0.0F` | Rolling average of ping over history window. |
| `minPing` | `float` | `999.0F` | Lowest ping observed. |
| `maxPing` | `float` | `0.0F` | Highest ping observed. |
| `jitter` | `float` | `0.0F` | Standard deviation of ping (latency variance). |
| `packetsSent` | `uint32_t` | `0` | Total packets sent since connection. |
| `packetsReceived` | `uint32_t` | `0` | Total packets received since connection. |
| `packetsLost` | `uint32_t` | `0` | Estimated packets lost (sent - received). |
| `packetLossRate` | `float` | `0.0F` | Packet loss as percentage (0-100). |
| `bandwidthIn` | `float` | `0.0F` | Incoming bandwidth in KB/s. |
| `bandwidthOut` | `float` | `0.0F` | Outgoing bandwidth in KB/s. |
| `lastUpdateTime` | `float` | `0.0F` | Timestamp of last statistics update. |
| `timeSinceUpdate` | `float` | `0.0F` | Time since last sample was added. |
| `pingHistory` | `deque<float>` | `{}` | Rolling buffer of ping samples. |
| `bytesInHistory` | `deque<uint32_t>` | `{}` | Rolling buffer of incoming byte counts. |
| `bytesOutHistory` | `deque<uint32_t>` | `{}` | Rolling buffer of outgoing byte counts. |

### Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `kMaxHistorySize` | `60` | Maximum samples in history buffers (1 minute at 1 sample/sec). |

## Methods

### `create()`

Factory method for creating a new stats component.

```cpp
static NetworkStatsComponent create()
{
    return NetworkStatsComponent{};
}
```

### `addPingSample(float ping)`

Records a new ping measurement and updates statistics.

```cpp
void addPingSample(float ping)
{
    currentPing = ping;
    pingHistory.push_back(ping);
    if (pingHistory.size() > kMaxHistorySize) {
        pingHistory.pop_front();
    }

    // Update min/max
    if (ping < minPing) minPing = ping;
    if (ping > maxPing) maxPing = ping;

    // Calculate average
    float sum = 0.0F;
    for (float p : pingHistory) {
        sum += p;
    }
    averagePing = sum / static_cast<float>(pingHistory.size());

    // Calculate jitter (standard deviation)
    if (pingHistory.size() > 1) {
        float variance = 0.0F;
        for (float p : pingHistory) {
            float diff = p - averagePing;
            variance += diff * diff;
        }
        jitter = std::sqrt(variance / static_cast<float>(pingHistory.size()));
    }
}
```

### `addBandwidthSample(uint32_t bytesIn, uint32_t bytesOut)`

Records bandwidth usage for the sample period.

```cpp
void addBandwidthSample(std::uint32_t bytesIn, std::uint32_t bytesOut)
{
    bytesInHistory.push_back(bytesIn);
    bytesOutHistory.push_back(bytesOut);

    if (bytesInHistory.size() > kMaxHistorySize) {
        bytesInHistory.pop_front();
    }
    if (bytesOutHistory.size() > kMaxHistorySize) {
        bytesOutHistory.pop_front();
    }

    // Calculate average bandwidth in KB/s
    std::uint32_t totalIn = 0;
    std::uint32_t totalOut = 0;

    for (std::uint32_t b : bytesInHistory) {
        totalIn += b;
    }
    for (std::uint32_t b : bytesOutHistory) {
        totalOut += b;
    }

    bandwidthIn = static_cast<float>(totalIn) / 1024.0F;
    bandwidthOut = static_cast<float>(totalOut) / 1024.0F;
}
```

### `updatePacketLoss()`

Calculates packet loss from sent/received counts.

```cpp
void updatePacketLoss()
{
    if (packetsSent > 0) {
        packetsLost = packetsSent - packetsReceived;
        packetLossRate = (static_cast<float>(packetsLost) / 
                          static_cast<float>(packetsSent)) * 100.0F;
    }
}
```

## Usage Patterns

### Setting Up Network Stats

```cpp
Entity createNetworkStatsEntity(Registry& registry)
{
    auto entity = registry.createEntity();
    registry.addComponent(entity, NetworkStatsComponent::create());
    return entity;
}
```

### Recording Ping from Server Response

```cpp
void NetworkReceiver::onPingResponse(Registry& registry, float sendTime)
{
    float currentTime = getTime();
    float ping = (currentTime - sendTime) * 1000.0F;  // Convert to ms
    
    auto view = registry.view<NetworkStatsComponent>();
    for (auto entity : view) {
        auto& stats = view.get<NetworkStatsComponent>(entity);
        stats.addPingSample(ping);
    }
}
```

### Tracking Packets

```cpp
void NetworkSender::send(const Packet& packet, Registry& registry)
{
    m_socket.send(packet);
    
    auto view = registry.view<NetworkStatsComponent>();
    for (auto entity : view) {
        auto& stats = view.get<NetworkStatsComponent>(entity);
        stats.packetsSent++;
    }
}

void NetworkReceiver::receive(Registry& registry)
{
    auto packet = m_socket.receive();
    if (packet.valid) {
        auto view = registry.view<NetworkStatsComponent>();
        for (auto entity : view) {
            auto& stats = view.get<NetworkStatsComponent>(entity);
            stats.packetsReceived++;
            stats.updatePacketLoss();
        }
    }
}
```

### Bandwidth Monitoring

```cpp
void NetworkStatsSystem::update(Registry& registry, float deltaTime)
{
    auto view = registry.view<NetworkStatsComponent>();
    
    for (auto entity : view) {
        auto& stats = view.get<NetworkStatsComponent>(entity);
        stats.timeSinceUpdate += deltaTime;
        
        // Sample bandwidth every second
        if (stats.timeSinceUpdate >= 1.0F) {
            std::uint32_t bytesIn = m_receiver.getBytesReceivedThisSecond();
            std::uint32_t bytesOut = m_sender.getBytesSentThisSecond();
            
            stats.addBandwidthSample(bytesIn, bytesOut);
            stats.timeSinceUpdate = 0.0F;
            
            m_receiver.resetByteCounter();
            m_sender.resetByteCounter();
        }
    }
}
```

## HUD Display

Rendering network stats for player visibility:

```cpp
void HUDSystem::renderNetworkStats(const NetworkStatsComponent& stats, Window& window)
{
    // Connection quality indicator
    Color qualityColor = getConnectionQualityColor(stats);
    drawCircle(SCREEN_WIDTH - 30, 30, 10, qualityColor);
    
    // Detailed stats (if debug enabled)
    if (m_showDetailedStats) {
        std::stringstream ss;
        ss << "Ping: " << static_cast<int>(stats.currentPing) << "ms";
        ss << " (avg: " << static_cast<int>(stats.averagePing) << ")";
        drawText(ss.str(), SCREEN_WIDTH - 200, 50, 14, Color{200, 200, 200});
        
        ss.str("");
        ss << "Jitter: " << std::fixed << std::setprecision(1) << stats.jitter << "ms";
        drawText(ss.str(), SCREEN_WIDTH - 200, 70, 14, Color{200, 200, 200});
        
        ss.str("");
        ss << "Loss: " << std::fixed << std::setprecision(1) << stats.packetLossRate << "%";
        drawText(ss.str(), SCREEN_WIDTH - 200, 90, 14, Color{200, 200, 200});
        
        ss.str("");
        ss << "In: " << std::fixed << std::setprecision(1) << stats.bandwidthIn << " KB/s";
        ss << " Out: " << stats.bandwidthOut << " KB/s";
        drawText(ss.str(), SCREEN_WIDTH - 200, 110, 14, Color{200, 200, 200});
    }
}

Color HUDSystem::getConnectionQualityColor(const NetworkStatsComponent& stats)
{
    // Excellent: < 50ms, < 5% loss
    if (stats.averagePing < 50 && stats.packetLossRate < 5) {
        return Color{50, 205, 50};   // Green
    }
    // Good: < 100ms, < 10% loss
    if (stats.averagePing < 100 && stats.packetLossRate < 10) {
        return Color{255, 215, 0};   // Yellow
    }
    // Moderate: < 200ms, < 20% loss
    if (stats.averagePing < 200 && stats.packetLossRate < 20) {
        return Color{255, 140, 0};   // Orange
    }
    // Poor
    return Color{255, 69, 0};        // Red
}
```

## Debug Overlay

Full network diagnostics:

```cpp
void NetworkDebugOverlay::render(const NetworkStatsComponent& stats)
{
    const float startY = 100;
    const float lineHeight = 20;
    float y = startY;
    
    drawText("=== Network Diagnostics ===", 10, y, 16, Color{255, 255, 100});
    y += lineHeight * 1.5F;
    
    drawText("Latency:", 10, y, 14, Color{180, 180, 180}); y += lineHeight;
    drawText("  Current: " + std::to_string(stats.currentPing) + " ms", 10, y, 12, Color{255, 255, 255});
    y += lineHeight;
    drawText("  Average: " + std::to_string(stats.averagePing) + " ms", 10, y, 12, Color{255, 255, 255});
    y += lineHeight;
    drawText("  Min/Max: " + std::to_string(stats.minPing) + "/" + 
             std::to_string(stats.maxPing) + " ms", 10, y, 12, Color{255, 255, 255});
    y += lineHeight;
    drawText("  Jitter: " + std::to_string(stats.jitter) + " ms", 10, y, 12, Color{255, 255, 255});
    y += lineHeight * 1.5F;
    
    drawText("Packets:", 10, y, 14, Color{180, 180, 180}); y += lineHeight;
    drawText("  Sent: " + std::to_string(stats.packetsSent), 10, y, 12, Color{255, 255, 255});
    y += lineHeight;
    drawText("  Received: " + std::to_string(stats.packetsReceived), 10, y, 12, Color{255, 255, 255});
    y += lineHeight;
    drawText("  Lost: " + std::to_string(stats.packetsLost) + 
             " (" + std::to_string(stats.packetLossRate) + "%)", 10, y, 12, Color{255, 100, 100});
    y += lineHeight * 1.5F;
    
    drawText("Bandwidth:", 10, y, 14, Color{180, 180, 180}); y += lineHeight;
    drawText("  In: " + std::to_string(stats.bandwidthIn) + " KB/s", 10, y, 12, Color{100, 255, 100});
    y += lineHeight;
    drawText("  Out: " + std::to_string(stats.bandwidthOut) + " KB/s", 10, y, 12, Color{255, 100, 100});
}
```

## Adaptive Quality

Using stats to adjust game settings:

```cpp
void adaptToNetworkConditions(const NetworkStatsComponent& stats, GameSettings& settings)
{
    // Reduce visual quality under poor conditions
    if (stats.averagePing > 150 || stats.packetLossRate > 15) {
        settings.interpolationTime *= 1.2F;  // More buffer for jitter
        settings.extrapolationEnabled = true;
        settings.maxExtrapolationTime = 0.3F;
    }
    
    // Adjust input send rate based on bandwidth
    if (stats.bandwidthOut > 50) {  // Using too much bandwidth
        settings.inputSendRate = 20;  // Reduce from 60 to 20 Hz
    }
}
```

## Best Practices

1. **Regular Sampling**: Update stats at consistent intervals (e.g., 1/second)
2. **History Size**: Balance between accuracy and memory usage
3. **Reset on Reconnect**: Clear stats when connection is re-established
4. **Visible Indicator**: Always show basic connection quality to players
5. **Log Anomalies**: Record unusual spikes for debugging

## Performance Considerations

- **Bounded History**: Fixed-size deques prevent memory growth
- **Efficient Statistics**: Rolling calculations avoid full history scans
- **Low Overhead**: Simple arithmetic operations only

## Related Components

- [InputHistoryComponent](input-history-component.md) - Input tracking for reconciliation

## Related Systems

- [NetworkStatsSystem](../systems/network-stats-system.md) - Updates stats each frame
- [NetworkDebugOverlay](../systems/network-debug-overlay.md) - Renders debug info
- [HUDSystem](../systems/hud-system.md) - Shows connection indicator
