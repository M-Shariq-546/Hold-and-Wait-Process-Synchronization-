#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_PROCESSES 4
#define MAX_LINESIZE 100

typedef struct {
    int pid;
    int type; // 0 for SEND, 1 for RECV
    int value;
    int sender_pid; // Used for matching operations
} ProcessOperation;

typedef struct {
    ProcessOperation operations[MAX_PROCESSES];
    int num_operations;
    bool is_blocked; // Add the is_blocked variable here
} ProcessQueue;

typedef struct {
    int sender_pid;
    int receiver_pid;
    int value;
} Message;

typedef struct {
    Message messages[MAX_PROCESSES];
    int front;
    int rear;
} MessageQueue;

ProcessQueue process_queues[MAX_PROCESSES];
int current_process = 0;

MessageQueue message_queue;

void init_process_queues() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_queues[i].num_operations = 0;
        process_queues[i].is_blocked = false; // Initialize is_blocked to false
    }
}

void enqueue_operation(ProcessQueue* queue, ProcessOperation operation) {
    if (queue->num_operations < MAX_PROCESSES) {
        queue->operations[queue->num_operations++] = operation;
    }
}

void send_message(int sender_pid, int receiver_pid, int value) {
    if (message_queue.rear < MAX_PROCESSES - 1) {
        Message message;
        message.sender_pid = sender_pid;
        message.receiver_pid = receiver_pid;
        message.value = value;

        message_queue.messages[message_queue.rear++] = message;
    }
}

Message receive_message(int receiver_pid) {
    Message empty_message;
    empty_message.sender_pid = -1;
    empty_message.receiver_pid = -1;
    empty_message.value = -1;

    if (message_queue.front == message_queue.rear) {
        return empty_message; // No messages in the queue
    }

    Message message = message_queue.messages[message_queue.front];
    if (message.receiver_pid == receiver_pid) {
        message_queue.front++;
        return message;
    }

    return empty_message;
}

void print_process_queues() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        printf("[");
        for (int j = 0; j < process_queues[i].num_operations; j++) {
            ProcessOperation operation = process_queues[i].operations[j];
            printf("Proc%d %s %d %d ", operation.pid + 1, (operation.type == 0) ? "SEND" : "RECV", operation.value, operation.sender_pid + 1);
        }
        printf("]");
    }
    printf("\n");
}

void print_message_queue() {
    printf("Message Queue: ");
    for (int i = message_queue.front; i < message_queue.rear; i++) {
        Message message = message_queue.messages[i];
        printf("[Sender: %d, Receiver: %d, Value: %d] ", message.sender_pid + 1, message.receiver_pid + 1, message.value);
    }
    printf("\n");
}

void run_process(ProcessOperation operation) {
    int target_pid = (operation.type == 0) ? operation.value - 1 : operation.sender_pid - 1;
    if (process_queues[target_pid].num_operations > 0) {
        ProcessOperation target_operation = process_queues[target_pid].operations[0];
        if ((operation.type == 0 && target_operation.type == 1 && operation.value == target_operation.value && operation.sender_pid == target_operation.pid) ||
            (operation.type == 1 && target_operation.type == 0 && operation.value == target_operation.value && operation.pid == target_operation.sender_pid)) {
            printf("Process %d matched with Process %d\n", current_process + 1, target_pid + 1);
            process_queues[target_pid].num_operations--;
            for (int i = 0; i < process_queues[target_pid].num_operations; i++) {
                process_queues[target_pid].operations[i] = process_queues[target_pid].operations[i + 1];
            }
            return;
        }
    }
    process_queues[current_process].is_blocked = true;
}

int get_user_input(char* line) {
    printf("Enter the process operation (e.g., Proc1 0 2 2): ");
    return fgets(line, MAX_LINESIZE, stdin) != NULL;
}

int main() {
    init_process_queues();
    message_queue.front = 0;
    message_queue.rear = 0;

    char line[MAX_LINESIZE];

    while (get_user_input(line)) {
        if (strncmp(line, "HALT", 4) == 0) {
            break;
        }

        ProcessOperation operation;
        sscanf(line, "Proc%d %d %d %d", &operation.pid, &operation.type, &operation.value, &operation.sender_pid);
        operation.pid--;

        if (operation.type == 0) { // SEND operation
            send_message(operation.pid, operation.sender_pid, operation.value);
        } else if (operation.type == 1) { // RECV operation
            Message received_message = receive_message(operation.pid);
            if (received_message.sender_pid != -1) {
                printf("Process %d received message from Process %d with value %d\n", operation.pid + 1, received_message.sender_pid + 1, received_message.value);
            }
        }

        enqueue_operation(&process_queues[operation.pid], operation);

        printf("[%02d] ", current_process + 1);
        print_process_queues();
        print_message_queue();

        ProcessQueue current_queue = process_queues[current_process];
        while (current_queue.num_operations > 0 && current_queue.operations[0].type != current_queue.operations[0].sender_pid) {
            ProcessOperation current_operation = current_queue.operations[0];
            process_queues[current_process].num_operations--;
            for (int i = 0; i < process_queues[current_process].num_operations; i++) {
                process_queues[current_process].operations[i] = process_queues[current_process].operations[i + 1];
            }
            run_process(current_operation);
            current_queue = process_queues[current_process];
        }

        process_queues[current_process].is_blocked = false; // Clear the is_blocked flag

        current_process = (current_process + 1) % MAX_PROCESSES;
    }

    printf("All producers and consumers have finished. Program terminated.\n");
    print_message_queue();

    return 0;
}
