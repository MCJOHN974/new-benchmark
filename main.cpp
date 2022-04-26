#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <sys/time.h>
#include <utility>


class Timer {
   timespec begin;
   timespec end;

public:
    void Start() {
        clock_gettime(CLOCK_REALTIME, &begin); 
    }
    void Finish() {
        clock_gettime(CLOCK_REALTIME, &end);
    }
    long Seconds() {
        return end.tv_sec - begin.tv_sec;
    }
    long NSeconds() {
        return end.tv_nsec - begin.tv_nsec;
    }
};


class SizeGenerator {
public:
    virtual int size() const = 0;
    virtual ~SizeGenerator() = default;
    virtual std::string Info() {
        return "Information about generator of buffer sizes:\n";
    }
};

class ByteGenerator : public SizeGenerator {
public:
    int size() const override {return rand() % 1024;}
    std::string Info() override {
        return SizeGenerator::Info() + "Uniform generation in [0-1Kb)\n";
    }
};

class KbGenerator : public SizeGenerator {
public:
    int size() const override {
        ByteGenerator generator;
        return generator.size() + generator.size() * 1024;
    }
    std::string Info() override {
        return SizeGenerator::Info() + "Uniform generation in [0-1Mb)\n";
    }
};

class MbGenerator : public SizeGenerator {
public:
    int size() const override {
        KbGenerator kgenerator;
        ByteGenerator bgenerator;
        return 1024 * 1024 * bgenerator.size() + kgenerator.size();
    }
    std::string Info() override {
        return SizeGenerator::Info() + "Uniform generation in [0-1Gb)\n";
    }
};

class EqualGenerator : public SizeGenerator {
public:
    int size() const override {
        int x = rand() % 3;
        if (x == 0) {
            ByteGenerator gen;
            return gen.size();
        }
        if (x == 1) {
            KbGenerator gen;
            return gen.size();
        }
        MbGenerator gen;
        return gen.size();
    }
    std::string Info() override {
        return SizeGenerator::Info() + "Equal probabilyty of three sizes: few bytes, few Kb or few Mb\n";
    }
};


struct Report {
    long alloc_count;
    long seconds;
    long nseconds;
    std::string info;
};

class Reporter {
public:
    virtual void DoReport() = 0;
    virtual ~Reporter() = default;
};

class StdioReporter : public Reporter {
    Report report;

public:
    StdioReporter(const Report& r) {
        report = r;
    }
    void DoReport() override {
        std::cout << report.info;
        std::cout << "Number of allocations/deallocations = " << report.alloc_count << '\n';
        std::cout << "Total time: " << report.seconds <<
        " seconds, " << report.nseconds << " nseconds.\n";
    }
};

class Benchmark {
public:
    virtual Report Result() = 0;
    virtual std::string Info() {
        return "Benchmark info:\n";
    }
    virtual ~Benchmark() = default;
};

class TestBuffer {
public:
    virtual void Realloc(int size) = 0;
    virtual ~TestBuffer() = default;
};

class RealBuffer : public TestBuffer {
    char* buffer_;
    bool allocated_;

public:
    void Realloc(int size) override {
        if (allocated_) {
            delete[] buffer_;
        } else {
            buffer_ = new char[size];
        }
        allocated_ = !allocated_;
    }
    ~RealBuffer() {
        if(allocated_) {
            delete[] buffer_;
        }
    }
};

class FakeBuffer : public TestBuffer {
    char* buffer_;
    bool allocated_;

public:
    void Realloc(int size) override {
        if (allocated_) {
            (void)buffer_;
        } else {
            (void)buffer_;
        }
        allocated_ = !allocated_;
    }
};


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
        timer.Start(); // validity threat: i think in  case of fake buffer -O3 will remove this part at all
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

int main() {
    int total_nsek_per_alloc;
    for(int i = 0; i < 10; ++i) {
        SizeGenerator *generator = new EqualGenerator();
        Benchmark* bench = new StandartBenchmark(1000, 100000, generator);
        Report report = bench->Result();

        total_nsek_per_alloc += (report.nseconds / report.alloc_count);
        Reporter *reporter = new StdioReporter(report);
        reporter->DoReport();
        delete bench;
    }
    std::cout << total_nsek_per_alloc / 10 << '\n';
    return 0;
}