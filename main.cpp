#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <queue>

using namespace std;

class Process {
public:
    string job;
    int p_no = 0, arrivalTime = 0, burstTime = 0, priority = 0;
    int finishTime = 0, turnaroundTime = 0, waitingTime = 0;
    int remainingTime = 0, CT = 0, RT = 0, start_AT = 0, BT_left = 0, temp_BT = 0;

    Process(const string& j = "", int at = 0, int bt = 0, int p = 0)
        : job(j), arrivalTime(at), burstTime(bt), priority(p),
          remainingTime(bt), BT_left(bt), start_AT(at) {}

    void calculate() {
        turnaroundTime = finishTime - start_AT;
        waitingTime = turnaroundTime - burstTime;
        CT = finishTime;
        RT = RT == 0 ? finishTime - start_AT : RT;
    }

    static bool compareArrival(const Process& a, const Process& b) {
        return a.arrivalTime > b.arrivalTime;
    }
};

class GanttChart {
private:
    struct Step {
        string job;
        int start;
        int stop;
        int p_no;

        Step(const string& j, int s, int e, int pno = 0)
            : job(j), start(s), stop(e), p_no(pno) {}
    };

    vector<Step> steps;

public:
    void clear() { steps.clear(); }

    void addStep(const string& job, int start, int stop, int pno = 0) {
        steps.emplace_back(job, start, stop, pno);
    }

    void display(bool isRoundRobin = false) const {
        if (steps.empty()) return;

        vector<Step> mergedSteps;
        for (const auto& step : steps) {
            if (mergedSteps.empty() ||
                mergedSteps.back().job != step.job ||
                mergedSteps.back().p_no != step.p_no) {
                mergedSteps.push_back(step);
            } else {
                mergedSteps.back().stop = step.stop;
            }
        }

        cout << "\nGantt Chart:\n+";
        for (const auto& step : mergedSteps) {
            if (!isRoundRobin) {
                cout << string(10, '-') << "+";
            } else {
                string processName = step.p_no == -1 ? "IS" : "P" + to_string(step.p_no);
                cout << string(processName.length() + 4, '-') << "+";
            }
        }
        cout << "\n|";

        for (const auto& step : mergedSteps) {
            if (!isRoundRobin) {
                cout << setw(9) << step.job << " |";
            } else {
                string processName = step.p_no == -1 ? "IS" : "P" + to_string(step.p_no);
                cout << " " << processName << " |";
            }
        }
        cout << "\n+";

        for (const auto& step : mergedSteps) {
            if (!isRoundRobin) {
                cout << string(10, '-') << "+";
            } else {
                string processName = step.p_no == -1 ? "IS" : "P" + to_string(step.p_no);
                cout << string(processName.length() + 4, '-') << "+";
            }
        }
        cout << "\n";

        cout << mergedSteps[0].start;
        for (const auto& step : mergedSteps) {
            cout << setw(10) << step.stop << " ";
        }
        cout << "\n";
    }
};

class SchedulingManager {
private:
    vector<Process> processes;
    GanttChart ganttChart;

    void inputProcesses() {
        int n;
        cout << "Enter number of processes: ";
        cin >> n;
        processes.clear();

        for (int i = 0; i < n; i++) {
            string job = "P" + to_string(i + 1);
            Process p(job);
            p.p_no = i + 1;

            cout << "\nProcess " << job << ":\n";
            cout << "Arrival Time: ";
            cin >> p.arrivalTime;
            p.start_AT = p.arrivalTime;

            cout << "Burst Time: ";
            cin >> p.burstTime;
            p.BT_left = p.remainingTime = p.burstTime;

            if (scheduling_choice == 1) {
                cout << "Priority (lower number means higher priority): ";
                cin >> p.priority;
            }

            processes.push_back(p);
        }

        sort(processes.begin(), processes.end(),
            [](const Process& p1, const Process& p2) {
                return p1.arrivalTime < p2.arrivalTime;
            });
    }

    vector<Process> priorityScheduling() {
        vector<Process> completedProcesses;
        vector<Process> unfinishedJobs = processes;
        vector<Process> readyQueue;

        int currentTime = processes[0].arrivalTime;

        while (!unfinishedJobs.empty() || !readyQueue.empty()) {
            while (!unfinishedJobs.empty() &&
                   unfinishedJobs.front().arrivalTime <= currentTime) {
                readyQueue.push_back(unfinishedJobs.front());
                unfinishedJobs.erase(unfinishedJobs.begin());
            }

            if (readyQueue.empty()) {
                currentTime = unfinishedJobs.front().arrivalTime;
                continue;
            }

            auto it = min_element(readyQueue.begin(), readyQueue.end(),
                [](const Process& p1, const Process& p2) {
                    return p1.priority < p2.priority;
                });

            Process& current = *it;
            int timeSlice = 1;
            current.remainingTime -= timeSlice;

            ganttChart.addStep(current.job, currentTime, currentTime + timeSlice);

            if (current.remainingTime == 0) {
                current.finishTime = currentTime + timeSlice;
                current.calculate();
                completedProcesses.push_back(current);
                readyQueue.erase(it);
            }

            currentTime += timeSlice;
        }
        return completedProcesses;
    }

    vector<Process> roundRobinScheduling() {
        int timeQuantum;
        cout << "Enter Time Quantum: ";
        cin >> timeQuantum;
        ganttChart.clear();

        priority_queue<Process, vector<Process>, decltype(&Process::compareArrival)>
            readyQueue(Process::compareArrival);

        for (auto& p : processes) readyQueue.push(p);

        vector<Process> completedProcesses;
        int clock = 0;

        if (!readyQueue.empty() && readyQueue.top().arrivalTime > 0) {
            Process idleProcess;
            idleProcess.p_no = -1;
            idleProcess.temp_BT = readyQueue.top().arrivalTime;
            idleProcess.CT = readyQueue.top().arrivalTime;
            ganttChart.addStep("Idle", 0, readyQueue.top().arrivalTime);
            clock = readyQueue.top().arrivalTime;
        }

        while (!readyQueue.empty()) {
            Process p = readyQueue.top();
            readyQueue.pop();

            int remainingTimeQuantum = timeQuantum;

            while (remainingTimeQuantum > 0 && p.BT_left > 0) {
                int executionTime = min(remainingTimeQuantum, p.BT_left);
                p.BT_left -= executionTime;
                clock += executionTime;
                remainingTimeQuantum -= executionTime;

                ganttChart.addStep(p.job, clock - executionTime, clock, p.p_no);
            }

            if (p.BT_left == 0) {
                p.finishTime = clock;
                p.calculate();
                completedProcesses.push_back(p);
            } else {
                p.arrivalTime = clock;
                readyQueue.push(p);
            }
        }

        return completedProcesses;
    }

    void displayResults(const vector<Process>& completedProcesses, bool isRoundRobin = false) {
        if (completedProcesses.empty()) return;

        cout << "\nProcess Details:\n";
        cout << setw(10) << "Process"
             << setw(15) << "Arrival Time"
             << setw(12) << "Burst Time"
             << setw(15) << "Finish Time"
             << setw(15) << "Turnaround Time"
             << setw(15) << "Waiting Time"
             << setw(15) << "Response Time" << endl;

        double totalTAT = 0, totalWT = 0, totalRT = 0;
        for (const auto& p : completedProcesses) {
            cout << setw(10) << p.job
                 << setw(15) << p.start_AT
                 << setw(12) << p.burstTime
                 << setw(15) << p.finishTime
                 << setw(15) << p.turnaroundTime
                 << setw(15) << p.waitingTime
                 << setw(15) << p.RT << endl;

            totalTAT += p.turnaroundTime;
            totalWT += p.waitingTime;
            totalRT += p.RT;
        }

        int n = completedProcesses.size();
        cout << "\nAverage Turnaround Time: " << totalTAT/n
             << "\nAverage Waiting Time: " << totalWT/n
             << "\nAverage Response Time: " << totalRT/n << endl;

        ganttChart.display(isRoundRobin);
    }

public:
    int scheduling_choice;

    void run() {
        cout << "\n--- CPU Scheduling Simulator ---\n";
        cout << "1. Priority Scheduling\n";
        cout << "2. Round Robin Scheduling\n";
        cout << "3. Exit\n";
        cout << "Enter your choice: ";
        cin >> scheduling_choice;

        inputProcesses();
        vector<Process> results;

        if (scheduling_choice == 1) {
            results = priorityScheduling();
            displayResults(results);
        } else if(scheduling_choice == 2){
            results = roundRobinScheduling();
            displayResults(results, true);
        }
        else {
            return;
        }
    }
};

int main() {
    SchedulingManager scheduler;
    scheduler.run();
    return 0;
}
