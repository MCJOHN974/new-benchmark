class StandartBenchmark : public Benchmark {
    int num_of_buffers_;
    int num_of_allocations_;
    SizeGenerator* generator_;
    std::string info_;

public:
    StandartBenchmark(int bufs, int allocs, SizeGenerator *gen) {
        num_of_allocations_ = allocs;
        num_of_buffers_ = bufs;
        generator_ = gen; gen = nullptr;
    }

    Report Result() override {
        Report real_report = Run(true);
        Report fake_report = Run(false);
        real_report.info = generator_->Info() + Info();
        real_report.seconds -= fake_report.seconds;
        real_report.nseconds -= fake_report.nseconds;
        return real_report;
    }

    std::string Info() override {
        return Benchmark::Info() + "Standart benchmark with random buffer allocate/deallocate.\n";
    }

    ~StandartBenchmark() {
        delete generator_;
    }

private:
    Report Run(bool real) {
        Report report;
        std::vector<TestBuffer*> buffers(num_of_buffers_);
        if (real) {
            for(auto& ptr : buffers) {
                ptr = new RealBuffer();
            }
        } else {
            for(auto& ptr : buffers) {
                ptr = new FakeBuffer();
            }
        }
        Timer timer;
        timer.Start();
        for(int i = 0; i < num_of_allocations_; ++i) {
            buffers[rand() % num_of_buffers_]->Realloc(generator_->size());
        }
        timer.Finish();
        report.alloc_count = num_of_allocations_;
        report.seconds = timer.Seconds();
        report.nseconds = timer.NSeconds();
        return report;
    }
};

