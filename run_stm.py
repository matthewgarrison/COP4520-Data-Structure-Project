import os

executable_path = "bench/stm_vectorSSB64"
ratios = [ "25 25 25 25", "60 20 10 10", "40, 20, 40, 0" ];
transaction_sizes = ["1", "2", "4"]
thread_counts = ["1", "2", "4", "8"]

# pass in all possible combinations of transaction size, operation ratio, and thread count
for transaction_size in transaction_sizes:
    for ratio in ratios:
        for thread_count in thread_counts:
            # call executable, passing in thread count, transaction size, and thread count
            os.system(executable_path + " " + thread_count + " " + transaction_size + " " + ratio)
