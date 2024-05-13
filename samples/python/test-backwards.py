import time

def calculate_factorial(n):
    result = 1
    for i in range(1, n + 1):
        result *= i
    return result

def main():
    target_value = 120
    for i in range(1, 10):
        factorial = calculate_factorial(i)
        print(f"Factorial of {i} is {factorial}")
        time.sleep(1)  # Simulating some long-running process
        if factorial == target_value:
            print("Target value reached!")
            break
    else:
        assert False, f"Target value {target_value} not reached!"

if __name__ == "__main__":
    main()
