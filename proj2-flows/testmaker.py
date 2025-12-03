import random

def generate_valid_input():
    flow_count = random.randint(1, 10)
    flows = [f"{i+1} {random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(0, 255)} "
             f"{random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(0, 255)} "
             f"{random.randint(1000, 10000)} {random.randint(1, 100)} {random.randint(1, 100)} {random.uniform(0.01, 1.0):.2f}"
             for i in range(flow_count)]
    return f"count={flow_count}\n" + "\n".join(flows)

def generate_invalid_count():
    return "count=fgijwogjwe\n"

def generate_missing_count():
    return "\n".join([
        f"{i+1} 192.168.1.{i+1} 192.168.2.{i+1} 1000 10 20 0.05"
        for i in range(4)
    ])

def generate_invalid_ip():
    return "count=2\n" + "\n".join([
        "1 999.999.999.999 192.168.1.2 1000 10 20 0.05",
        "2 192.168.1.1 256.256.256.256 2000 20 30 0.07"
    ])

def generate_missing_fields():
    return "count=3\n" + "\n".join([
        "1 192.168.1.1 192.168.1.2 1000 10 20",
        "2 192.168.1.1 192.168.1.2 1000 10",    
        "3 192.168.1.1 192.168.1.2"            
    ])

def generate_zero_packets():
    return "count=2\n" + "\n".join([
        "1 192.168.1.1 192.168.1.2 1000 10 0 0.05", 
        "2 192.168.1.1 192.168.1.2 2000 20 30 0.07"
    ])

def generate_large_input():
    flow_count = 100000
    flows = [f"{i+1} {random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(0, 255)} "
             f"{random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(0, 255)} "
             f"{random.randint(1000, 10000)} {random.randint(1, 100)} {random.randint(1, 100)} {random.uniform(0.01, 1.0):.2f}"
             for i in range(flow_count)]
    return f"count={flow_count}\n" + "\n".join(flows)

def main():
    with open("valid_input.txt", "w") as f:
        f.write(generate_valid_input())
    with open("invalid_count.txt", "w") as f:
        f.write(generate_invalid_count())
    with open("missing_count.txt", "w") as f:
        f.write(generate_missing_count())
    with open("invalid_ip.txt", "w") as f:
        f.write(generate_invalid_ip())
    with open("missing_fields.txt", "w") as f:
        f.write(generate_missing_fields())
    with open("zero_packets.txt", "w") as f:
        f.write(generate_zero_packets())
    with open("large_input.txt", "w") as f:
        f.write(generate_large_input())

if __name__ == "__main__":
    main()