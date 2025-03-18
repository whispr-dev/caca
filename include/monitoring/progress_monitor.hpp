// File: include/caca/monitoring/progress_monitor.hpp
#pragma once

#include <string>
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <iomanip>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <optional>
#include <memory>
#include <cmath>

// Optional JSON library dependency
#ifdef CACA_USE_JSON
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace caca::monitoring {

/**
 * @class ProgressMonitor
 * @brief Provides real-time progress tracking and resource monitoring
 *
 * This class offers sophisticated progress tracking capabilities with
 * estimated completion time calculations, resource usage monitoring,
 * and multiple output formats.
 */
class ProgressMonitor {
public:
    /**
     * @struct ResourceUsage
     * @brief Tracks system resource utilization
     */
    struct ResourceUsage {
        double cpu_percent = 0.0;
        size_t memory_used_mb = 0;
        size_t total_memory_mb = 0;
        double gpu_percent = 0.0;
        double disk_io_mb_per_sec = 0.0;
    };

    /**
     * @struct TaskProgress
     * @brief Tracks progress of a specific task
     */
    struct TaskProgress {
        std::string task_name;
        double percent_complete = 0.0;
        std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point estimated_completion_time;
        size_t processed_items = 0;
        size_t total_items = 0;
        bool completed = false;
    };

    /**
     * @enum ProgressStyle
     * @brief Display style for console progress indicators
     */
    enum class ProgressStyle {
        Basic,      // Simple percentage and progress bar
        Detailed,   // Detailed with time estimates and resource usage
        Minimal,    // Minimal text-only display
        Custom      // Custom formatting
    };

private:
    // Storage for all tracked tasks
    std::unordered_map<std::string, TaskProgress> m_tasks;
    
    // Thread for continuous monitoring
    std::unique_ptr<std::thread> m_monitor_thread;
    
    // Control flags
    std::atomic<bool> m_stop_monitoring = false;
    std::atomic<bool> m_pause_monitoring = false;
    
    // Thread synchronization
    std::mutex m_tasks_mutex;
    std::condition_variable m_cv;
    
    // Display settings
    ProgressStyle m_style = ProgressStyle::Detailed;
    bool m_use_colors = true;
    bool m_show_spinner = true;
    bool m_auto_clear = true;
    int m_update_interval_ms = 100;
    
    // Output streams
    std::ostream* m_console_stream = &std::cout;
    std::ofstream m_log_file;
    
    // Custom formatter
    std::function<std::string(const TaskProgress&, const ResourceUsage&)> m_custom_formatter;
    
    // Spinner animation frames
    const std::vector<std::string> m_spinner_frames = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    size_t m_spinner_index = 0;

    /**
     * @brief Get current system resource usage
     * @return ResourceUsage structure with current usage metrics
     */
    ResourceUsage getSystemResources() {
        ResourceUsage resources;
        
        // Platform-specific resource measurement would go here
        // For now, just return some placeholder values
        resources.cpu_percent = getCpuUsage();
        resources.memory_used_mb = getMemoryUsage();
        resources.total_memory_mb = getTotalMemory();
        
        return resources;
    }
    
    /**
     * @brief Measure current CPU usage
     * @return CPU usage percentage
     */
    double getCpuUsage() {
        // Platform-specific implementation would go here
        // Placeholder implementation
        return 50.0; // 50% CPU usage
    }
    
    /**
     * @brief Measure current memory usage
     * @return Memory usage in MB
     */
    size_t getMemoryUsage() {
        // Platform-specific implementation would go here
        // Placeholder implementation
        return 1024; // 1GB memory usage
    }
    
    /**
     * @brief Get total system memory
     * @return Total memory in MB
     */
    size_t getTotalMemory() {
        // Platform-specific implementation would go here
        // Placeholder implementation
        return 16384; // 16GB total memory
    }
    
    /**
     * @brief Format the spinner animation
     * @return Current spinner frame
     */
    std::string getSpinnerFrame() {
        if (!m_show_spinner) {
            return "";
        }
        
        std::string frame = m_spinner_frames[m_spinner_index];
        m_spinner_index = (m_spinner_index + 1) % m_spinner_frames.size();
        return frame;
    }
    
    /**
     * @brief Format a time duration in human-readable form
     * @param duration Duration to format
     * @return Formatted string like "1h 23m 45s"
     */
    std::string formatDuration(std::chrono::seconds duration) {
        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
        duration -= hours;
        
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
        duration -= minutes;
        
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
        
        std::stringstream ss;
        if (hours.count() > 0) {
            ss << hours.count() << "h ";
        }
        if (hours.count() > 0 || minutes.count() > 0) {
            ss << minutes.count() << "m ";
        }
        ss << seconds.count() << "s";
        
        return ss.str();
    }
    
    /**
     * @brief Create a progress bar string
     * @param progress Progress value (0.0 to 1.0)
     * @param width Width of the progress bar
     * @return Formatted progress bar string
     */
    std::string createProgressBar(double progress, int width = 40) {
        int filled_width = static_cast<int>(progress * width);
        
        std::string bar;
        if (m_use_colors) {
            // Colorized version
            bar = "\033[1;32m";  // Green bold
            bar += std::string(filled_width, '█');
            bar += "\033[0;37m"; // White
            bar += std::string(width - filled_width, '░');
            bar += "\033[0m";    // Reset
        } else {
            // Non-colored version
            bar = std::string(filled_width, '█');
            bar += std::string(width - filled_width, '░');
        }
        
        return bar;
    }
    
    /**
     * @brief Format a progress using the specified style
     * @param progress Task progress
     * @param resources System resource usage
     * @return Formatted string
     */
    std::string formatProgress(const TaskProgress& progress, const ResourceUsage& resources) {
        if (m_custom_formatter) {
            return m_custom_formatter(progress, resources);
        }
        
        std::stringstream ss;
        
        switch (m_style) {
            case ProgressStyle::Minimal:
                ss << progress.task_name << ": "
                   << std::fixed << std::setprecision(1)
                   << (progress.percent_complete * 100.0) << "%";
                break;
                
            case ProgressStyle::Basic:
                ss << getSpinnerFrame() << " "
                   << progress.task_name << " "
                   << createProgressBar(progress.percent_complete) << " "
                   << std::fixed << std::setprecision(1)
                   << (progress.percent_complete * 100.0) << "%";
                break;
                
            case ProgressStyle::Detailed:
            default: {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    now - progress.start_time
                );
                
                std::optional<std::chrono::seconds> remaining;
                if (progress.percent_complete > 0.0) {
                    auto estimated_total = elapsed.count() / progress.percent_complete;
                    auto estimated_remaining = estimated_total - elapsed.count();
                    
                    if (estimated_remaining > 0) {
                        remaining = std::chrono::seconds(static_cast<int>(estimated_remaining));
                    }
                }
                
                ss << getSpinnerFrame() << " ";
                
                if (m_use_colors) {
                    ss << "\033[1;36m"; // Cyan bold
                }
                
                ss << progress.task_name;
                
                if (m_use_colors) {
                    ss << "\033[0m"; // Reset
                }
                
                ss << " " << createProgressBar(progress.percent_complete) << " "
                   << std::fixed << std::setprecision(1)
                   << (progress.percent_complete * 100.0) << "% "
                   << "| Items: " << progress.processed_items << "/" << progress.total_items
                   << " | CPU: " << std::fixed << std::setprecision(1) << resources.cpu_percent << "%"
                   << " | Mem: " << resources.memory_used_mb << "/" << resources.total_memory_mb << " MB"
                   << " | Time: " << formatDuration(elapsed);
                
                if (remaining) {
                    ss << " | ETA: " << formatDuration(remaining.value());
                } else {
                    ss << " | ETA: calculating...";
                }
                
                break;
            }
        }
        
        return ss.str();
    }
    
    /**
     * @brief Background monitoring thread function
     */
    void monitoringThread() {
        while (!m_stop_monitoring) {
            if (!m_pause_monitoring) {
                // Get current resources
                auto resources = getSystemResources();
                
                // Update progress display
                {
                    std::lock_guard<std::mutex> lock(m_tasks_mutex);
                    
                    if (!m_tasks.empty()) {
                        // Clear previous output if needed
                        if (m_auto_clear) {
                            *m_console_stream << "\033[2K\r"; // Clear line
                        }
                        
                        // Display latest progress
                        for (const auto& [name, progress] : m_tasks) {
                            if (!progress.completed) {
                                *m_console_stream << formatProgress(progress, resources) << std::endl;
                            }
                        }
                        
                        // Move cursor back up
                        if (m_auto_clear) {
                            size_t active_tasks = std::count_if(
                                m_tasks.begin(), m_tasks.end(),
                                [](const auto& pair) { return !pair.second.completed; }
                            );
                            
                            if (active_tasks > 1) {
                                *m_console_stream << "\033[" << (active_tasks - 1) << "A\r";
                            }
                        }
                        
                        m_console_stream->flush();
                    }
                }
                
                // Log to file if enabled
                if (m_log_file.is_open()) {
                    std::lock_guard<std::mutex> lock(m_tasks_mutex);
                    
                    for (const auto& [name, progress] : m_tasks) {
                        if (!progress.completed) {
                            logProgress(progress, resources);
                        }
                    }
                }
            }
            
            // Wait for next update
            std::unique_lock<std::mutex> lock(m_tasks_mutex);
            m_cv.wait_for(lock, std::chrono::milliseconds(m_update_interval_ms));
        }
    }
    
    /**
     * @brief Log progress to file
     * @param progress Task progress
     * @param resources System resource usage
     */
    void logProgress(const TaskProgress& progress, const ResourceUsage& resources) {
        #ifdef CACA_USE_JSON
        // JSON logging
        json log_entry = {
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
            {"task", progress.task_name},
            {"percent_complete", progress.percent_complete},
            {"processed_items", progress.processed_items},
            {"total_items", progress.total_items},
            {"cpu_usage", resources.cpu_percent},
            {"memory_used_mb", resources.memory_used_mb},
            {"total_memory_mb", resources.total_memory_mb}
        };
        
        m_log_file << log_entry.dump() << std::endl;
        #else
        // Simple CSV logging
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        
        m_log_file << now << ","
                   << progress.task_name << ","
                   << progress.percent_complete << ","
                   << progress.processed_items << ","
                   << progress.total_items << ","
                   << resources.cpu_percent << ","
                   << resources.memory_used_mb << ","
                   << resources.total_memory_mb
                   << std::endl;
        #endif
    }

public:
    /**
     * @brief Constructor
     * @param style Display style for progress indicators
     * @param use_colors Whether to use ANSI colors in output
     */
    ProgressMonitor(ProgressStyle style = ProgressStyle::Detailed, bool use_colors = true)
        : m_style(style), m_use_colors(use_colors) {
        // Start monitoring thread
        m_monitor_thread = std::make_unique<std::thread>(&ProgressMonitor::monitoringThread, this);
    }
    
    /**
     * @brief Destructor - ensures monitoring thread is stopped
     */
    ~ProgressMonitor() {
        // Stop monitoring thread
        m_stop_monitoring = true;
        m_cv.notify_all();
        
        if (m_monitor_thread && m_monitor_thread->joinable()) {
            m_monitor_thread->join();
        }
        
        // Close log file if open
        if (m_log_file.is_open()) {
            m_log_file.close();
        }
    }
    
    /**
     * @brief Update progress for a task
     * @param task_name Name of the task
     * @param processed_
     * items Number of items processed so far
     * @param total_items Total number of items to process
     */
    void update_progress(
        const std::string& task_name,
        size_t processed_items,
        size_t total_items
    ) {
        std::lock_guard<std::mutex> lock(m_tasks_mutex);
        
        // Get or create task
        TaskProgress& task = m_tasks[task_name];
        
        // Initialize if new
        if (task.task_name.empty()) {
            task.task_name = task_name;
            task.start_time = std::chrono::steady_clock::now();
        }
        
        // Update progress
        task.processed_items = processed_items;
        task.total_items = total_items;
        task.percent_complete = (total_items > 0) ? 
            static_cast<double>(processed_items) / total_items : 0.0;
        
        // Calculate estimated completion time
        if (processed_items > 0 && processed_items < total_items) {
            auto elapsed = std::chrono::steady_clock::now() - task.start_time;
            auto estimated_total = elapsed * (static_cast<double>(total_items) / processed_items);
            task.estimated_completion_time = task.start_time + estimated_total;
        }
        
        // Check if completed
        if (processed_items >= total_items) {
            task.completed = true;
            task.percent_complete = 1.0;
        }
        
        // Notify monitoring thread
        m_cv.notify_one();
    }
    
    /**
     * @brief Mark a task as completed
     * @param task_name Name of the task
     */
    void complete_task(const std::string& task_name) {
        std::lock_guard<std::mutex> lock(m_tasks_mutex);
        
        if (m_tasks.count(task_name) > 0) {
            auto& task = m_tasks[task_name];
            task.completed = true;
            task.percent_complete = 1.0;
            task.processed_items = task.total_items;
        }
        
        // Notify monitoring thread
        m_cv.notify_one();
    }
    
    /**
     * @brief Update progress by percentage
     * @param task_name Name of the task
     * @param percentage Percentage complete (0.0 to 1.0)
     */
    void update_progress_percentage(
        const std::string& task_name,
        double percentage
    ) {
        std::lock_guard<std::mutex> lock(m_tasks_mutex);
        
        // Get or create task
        TaskProgress& task = m_tasks[task_name];
        
        // Initialize if new
        if (task.task_name.empty()) {
            task.task_name = task_name;
            task.start_time = std::chrono::steady_clock::now();
        }
        
        // Update progress
        task.percent_complete = std::max(0.0, std::min(1.0, percentage));
        
        // Calculate estimated completion time
        if (task.percent_complete > 0.0 && task.percent_complete < 1.0) {
            auto elapsed = std::chrono::steady_clock::now() - task.start_time;
            auto estimated_total = elapsed * (1.0 / task.percent_complete);
            task.estimated_completion_time = task.start_time + estimated_total;
        }
        
        // Check if completed
        if (task.percent_complete >= 1.0) {
            task.completed = true;
        }
        
        // Notify monitoring thread
        m_cv.notify_one();
    }
    
    /**
     * @brief Pause progress monitoring temporarily
     */
    void pause() {
        m_pause_monitoring = true;
    }
    
    /**
     * @brief Resume progress monitoring
     */
    void resume() {
        m_pause_monitoring = false;
        m_cv.notify_one();
    }
    
    /**
     * @brief Enable logging to file
     * @param log_path Path to log file
     * @return True if logging was successfully enabled
     */
    bool enable_logging(const std::string& log_path) {
        // Close existing log file if open
        if (m_log_file.is_open()) {
            m_log_file.close();
        }
        
        // Open new log file
        m_log_file.open(log_path, std::ios::app);
        
        // Write header if file is empty
        if (m_log_file.tellp() == 0) {
            #ifdef CACA_USE_JSON
            // No header needed for JSON
            #else
            // CSV header
            m_log_file << "timestamp,task,percent_complete,processed_items,total_items,"
                       << "cpu_usage,memory_used_mb,total_memory_mb" << std::endl;
            #endif
        }
        
        return m_log_file.is_open();
    }
    
    /**
     * @brief Set custom progress formatter
     * @param formatter Function to format progress
     */
    void set_custom_formatter(
        std::function<std::string(const TaskProgress&, const ResourceUsage&)> formatter
    ) {
        m_custom_formatter = formatter;
        m_style = ProgressStyle::Custom;
    }
    
    /**
     * @brief Set display style
     * @param style Progress display style
     */
    void set_style(ProgressStyle style) {
        m_style = style;
    }
    
    /**
     * @brief Set color usage
     * @param use_colors Whether to use ANSI colors
     */
    void set_use_colors(bool use_colors) {
        m_use_colors = use_colors;
    }
    
    /**
     * @brief Set spinner visibility
     * @param show_spinner Whether to show animated spinner
     */
    void set_show_spinner(bool show_spinner) {
        m_show_spinner = show_spinner;
    }
    
    /**
     * @brief Set auto-clear behavior
     * @param auto_clear Whether to auto-clear previous output
     */
    void set_auto_clear(bool auto_clear) {
        m_auto_clear = auto_clear;
    }
    
    /**
     * @brief Set update interval
     * @param interval_ms Update interval in milliseconds
     */
    void set_update_interval(int interval_ms) {
        m_update_interval_ms = std::max(10, interval_ms);
    }
    
    /**
     * @brief Set output stream for console display
     * @param stream Output stream (nullptr to disable console output)
     */
    void set_console_stream(std::ostream* stream) {
        m_console_stream = stream;
    }
    
    /**
     * @brief Get all tasks
     * @return Copy of the tasks map
     */
    std::unordered_map<std::string, TaskProgress> get_tasks() const {
        std::lock_guard<std::mutex> lock(m_tasks_mutex);
        return m_tasks;
    }
    
    /**
     * @brief Get a specific task
     * @param task_name Name of the task
     * @return Optional TaskProgress, empty if task not found
     */
    std::optional<TaskProgress> get_task(const std::string& task_name) const {
        std::lock_guard<std::mutex> lock(m_tasks_mutex);
        
        auto it = m_tasks.find(task_name);
        if (it != m_tasks.end()) {
            return it->second;
        }
        
        return std::nullopt;
    }
    
    /**
     * @brief Reset a specific task
     * @param task_name Name of the task
     */
    void reset_task(const std::string& task_name) {
        std::lock_guard<std::mutex> lock(m_tasks_mutex);
        
        if (m_tasks.count(task_name) > 0) {
            TaskProgress& task = m_tasks[task_name];
            task.start_time = std::chrono::steady_clock::now();
            task.percent_complete = 0.0;
            task.processed_items = 0;
            task.completed = false;
        }
    }
    
    /**
     * @brief Remove a task
     * @param task_name Name of the task
     */
    void remove_task(const std::string& task_name) {
        std::lock_guard<std::mutex> lock(m_tasks_mutex);
        m_tasks.erase(task_name);
    }
    
    /**
     * @brief Clear all tasks
     */
    void clear_tasks() {
        std::lock_guard<std::mutex> lock(m_tasks_mutex);
        m_tasks.clear();
    }
};

} // namespace caca::monitoring

// File: src/monitoring/progress_monitor.cpp
#include "caca/monitoring/progress_monitor.hpp"

// Platform-specific implementations of resource monitoring

#if defined(_WIN32)
    // Windows implementation
    #include <windows.h>
    #include <psapi.h>

    namespace caca::monitoring {
        // Windows-specific resource monitoring would be implemented here
    }
#elif defined(__linux__)
    // Linux implementation
    #include <sys/resource.h>
    #include <sys/sysinfo.h>
    #include <fstream>
    #include <string>
    #include <cstring>

    namespace caca::monitoring {
        // Linux-specific resource monitoring would be implemented here
    }
#elif defined(__APPLE__)
    // macOS implementation
    #include <mach/mach.h>
    #include <sys/sysctl.h>

    namespace caca::monitoring {
        // macOS-specific resource monitoring would be implemented here
    }
#else
    // Generic fallback implementation
    namespace caca::monitoring {
        // Generic resource monitoring would be implemented here
    }
#endif