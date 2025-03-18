namespace caca::monitoring {
    class ProgressMonitor {
    public:
        // Nested classes for granular tracking
        struct ResourceUsage {
            double cpu_percent = 0.0;
            size_t memory_used_mb = 0;
            size_t total_memory_mb = 0;
            double gpu_percent = 0.0;
            double disk_io_mb_per_sec = 0.0;
        };

        struct TaskProgress {
            std::string task_name;
            double percent_complete = 0.0;
            std::chrono::steady_clock::time_point start_time;
            std::chrono::steady_clock::time_point estimated_completion_time;
            size_t processed_items = 0;
            size_t total_items = 0;
        };

        class ConsoleProgressRenderer {
        public:
            // Modern, informative console rendering
            void render(const TaskProgress& progress, const ResourceUsage& resources) {
                // Unicode block characters for smoother progress bar
                const char* block_chars[] = {" ", "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█"};
                
                // Compute progress bar
                int bar_width = 50;
                int filled_width = static_cast<int>(progress.percent_complete * bar_width);
                
                // Colorful, informative console output
                std::cout << "\033[2K\r"; // Clear current line
                std::cout << fmt::format(
                    "🔍 Task: {:<20} | "
                    "Progress: {:.2f}% {} | "
                    "CPU: {:.1f}% | Mem: {}/{} MB | "
                    "Est. Remaining: {:%M}m {:%S}s",
                    progress.task_name,
                    progress.percent_complete * 100.0,
                    std::string(filled_width, '█') + 
                    std::string(bar_width - filled_width, '░'),
                    resources.cpu_percent,
                    resources.memory_used_mb,
                    resources.total_memory_mb,
                    std::chrono::minutes(static_cast<int>(
                        std::chrono::duration_cast<std::chrono::seconds>(
                            progress.estimated_completion_time - 
                            std::chrono::steady_clock::now()
                        ).count() / 60
                    )),
                    std::chrono::seconds(static_cast<int>(
                        std::chrono::duration_cast<std::chrono::seconds>(
                            progress.estimated_completion_time - 
                            std::chrono::steady_clock::now()
                        ).count() % 60
                    ))
                );
                std::cout.flush();
            }
        };

        class JSONLogRenderer {
        public:
            void log(const TaskProgress& progress, const ResourceUsage& resources) {
                // Log in machine-readable format for potential later analysis
                json log_entry = {
                    {"timestamp", std::chrono::system_clock::now()},
                    {"task", progress.task_name},
                    {"percent_complete", progress.percent_complete},
                    {"processed_items", progress.processed_items},
                    {"total_items", progress.total_items},
                    {"cpu_usage", resources.cpu_percent},
                    {"memory_used_mb", resources.memory_used_mb},
                    {"total_memory_mb", resources.total_memory_mb}
                };
                
                // Write to log file
                std::ofstream log_file("caca_progress.jsonl", std::ios_base::app);
                log_file << log_entry.dump() << "\n";
            }
        };

        // Adaptive progress estimation
        void update_progress(
            const std::string& task_name, 
            size_t processed, 
            size_t total
        ) {
            TaskProgress& current_task = tasks_[task_name];
            current_task.task_name = task_name;
            current_task.processed_items = processed;
            current_task.total_items = total;
            current_task.percent_complete = 
                static_cast<double>(processed) / total;
            
            // Compute estimated completion
            auto elapsed = std::chrono::steady_clock::now() - current_task.start_time;
            if (processed > 0) {
                auto estimated_total_time = 
                    elapsed * (static_cast<double>(total) / processed);
                current_task.estimated_completion_time = 
                    current_task.start_time + estimated_total_time;
            }

            // Render progress
            ConsoleProgressRenderer console_renderer;
            JSONLogRenderer json_renderer;

            ResourceUsage current_resources = get_system_resources();
            
            console_renderer.render(current_task, current_resources);
            json_renderer.log(current_task, current_resources);
        }

    private:
        // Platform-specific resource gathering
        ResourceUsage get_system_resources() {
            // Placeholder - actual implementation would use 
            // platform-specific APIs like /proc on Linux, 
            // Windows Performance Counters, etc.
            ResourceUsage resources;
            
            // Simulated resource gathering
            resources.cpu_percent = get_cpu_usage();
            resources.memory_used_mb = get_memory_usage();
            resources.total_memory_mb = get_total_memory();
            
            return resources;
        }

        // Placeholder methods - to be implemented with platform-specific code
        double get_cpu_usage() { /* ... */ }
        size_t get_memory_usage() { /* ... */ }
        size_t get_total_memory() { /* ... */ }

        std::unordered_map<std::string, TaskProgress> tasks_;
    };
}