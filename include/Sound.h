/**
 * @file Sound.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief USE multi-platform sound handler.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 */
#ifndef USE_SOUND_H
#define USE_SOUND_H

#include <functional>
#include <memory>
#include "miniaudio.h"

#include "Types.h"

/**
 * USE sound manager.
 *
 * The Sound class handles all sounds produced by the components, mixes and plays them.
 * Multi-platform audio provided by miniaudio.h.
 * */
class Sound {

private:
    bool m_configured = false;
    bool m_running = false;

    // ===========================================
    // Audio parameters
    // ===========================================
    /// Number of PCM frames processed per second (two samples for stereo in a single frame).
    const int SAMPLE_RATE   = 44100;
    const int CHANNEL_COUNT = 2;

    // Low Pass Filter parameters.
    const int   LPF_CUTOFF_FREQ   = 45000;
    const int   LPF_ORDER           = 8;
    //const float LPF_BIAS            = 0.9f;

    // ===========================================
    // Miniaudio data source definition
    // ===========================================
    struct callbackDataSource_t {
        ma_data_source_base base;
        std::function<SoundStereoFrame(void)> sampleSource;
    };

    static ma_result callbackDataSourceRead(ma_data_source* pDataSource, void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead)
    {
        SoundStereoFrame frame = static_cast<callbackDataSource_t*>(pDataSource)->sampleSource();
        float *output = static_cast<float*>(pFramesOut);

        *output = frame.left;
        output++;
        *output = frame.right;

        if(pFramesRead)
            *pFramesRead = 1;

        return MA_SUCCESS;
    }

    static ma_result callbackDataSourceGetDataFormat(ma_data_source* pDataSource, ma_format* pFormat, ma_uint32* pChannels, ma_uint32* pSampleRate, ma_channel* pChannelMap, size_t channelMapCap)
    {
        *pFormat = ma_format_f32;
        *pChannels = 2;
        *pSampleRate = 44100;

        return MA_SUCCESS;
    }

    static ma_result callbackDataSourceSeek(ma_data_source* pDataSource, ma_uint64 frameIndex) { return MA_NOT_IMPLEMENTED; }
    static ma_result callbackDataSourceGetCursor(ma_data_source* pDataSource, ma_uint64* pCursor) { return MA_NOT_IMPLEMENTED; }
    static ma_result callbackDataSourceGetLength(ma_data_source* pDataSource, ma_uint64* pLength) { return MA_NOT_IMPLEMENTED; }

    constexpr static ma_data_source_vtable g_my_data_source_vtable {
            callbackDataSourceRead,
            callbackDataSourceSeek,
            callbackDataSourceGetDataFormat,
            callbackDataSourceGetCursor,
            callbackDataSourceGetLength
    };

    static ma_result callBackDataSourceInit(callbackDataSource_t* pMyDataSource, std::function<SoundStereoFrame(void)> callback)
    {
        ma_result result;
        ma_data_source_config baseConfig;

        baseConfig = ma_data_source_config_init();
        baseConfig.vtable = &g_my_data_source_vtable;

        result = ma_data_source_init(&baseConfig, &pMyDataSource->base);
        if (result != MA_SUCCESS) {
            return result;
        }

        pMyDataSource->sampleSource = std::move(callback);

        return MA_SUCCESS;
    }

    static void callBackDataSourceUninit(callbackDataSource_t* pMyDataSource)
    {
        // ... do the uninitialization of your custom data source here ...

        // You must uninitialize the base data source.
        ma_data_source_uninit(&pMyDataSource->base);
    }

    // ===========================================
    // Miniaudio data
    // ===========================================
    ma_device_config m_maConfig;
    ma_lpf_node_config  m_maLpfConfig;
    ma_node_graph_config  m_maNodeGraphConfig;
    static void deleteNodeGraph(ma_node_graph *ng);
    static void deleteLpfNode(ma_lpf_node *node);
    static void deleteDataNode(ma_data_source_node *node);
    // RAII encapsulation for miniaudio structs (they require custom deleter).
    // Warning: maintain the order of the members, so the destruction happens in the correct order too (see C++ standard 12.6.2).
    std::unique_ptr<ma_device, decltype(&ma_device_uninit)> m_maDevice{new ma_device, &ma_device_uninit};
    std::unique_ptr<ma_node_graph, decltype(&deleteNodeGraph)> m_maNodeGraph{new ma_node_graph, &deleteNodeGraph};
    std::unique_ptr<ma_lpf_node, decltype(&deleteLpfNode)> m_maNodeLpf{new ma_lpf_node, &deleteLpfNode};

    // The sound is processed in a following way:
    //  +-------------------+
    // |                   |
    // |  Sample source 1  |---+
    // |                   |   |
    // +-------------------|   |        +-------------------+
    //                         |        |                   |
    //         ...             +------->|  Low Pass Filter  |
    //                         |        |                   |
    // +-------------------|   |        +-------------------+
    // |                   |   |
    // |  Sample source x  |---+
    // |                   |
    // +-------------------+
    // These are the sample sources.
    std::vector<std::unique_ptr<callbackDataSource_t, decltype(&callBackDataSourceUninit)>> m_maDataSources;
    // These are the nodes encapsulating the sample sources.
    std::vector<std::unique_ptr<ma_data_source_node, decltype(&deleteDataNode)>> m_maNodesDataSource;

    static void dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

    // ===========================================
    // Current configuration (based on System's data)
    // ===========================================
    unsigned long m_expectedClockRate = 0;
    std::function<void(void)> m_systemClockFunction;

public:
    /**
     * Create sound device.
     * */
    Sound();
    ~Sound();

    void configureSound(SoundConfig cfg);
    void unloadConfig();
    [[nodiscard]] bool soundAvailable() const;

    void start();
    void stop();
};

#endif //USE_SOUND_H
