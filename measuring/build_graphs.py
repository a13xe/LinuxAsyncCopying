# pip install pandas matplotlib

import pandas as pd
import matplotlib.pyplot as plt

# Reading the CSV file
df = pd.read_csv('execution_times.csv')

# Setting the block size as the index
df.set_index('BlockSize(KB)', inplace=True)

# Plotting
plt.figure(figsize=(10, 6))
for column in df.columns:
    plt.plot(df.index, df[column], label=column)

plt.xlabel('Block Size (KB)')
plt.ylabel('Execution Time (Seconds)')
plt.title('Execution Time vs Block Size for Different Operations')
plt.legend(title='Number of Operations')
plt.grid(True)
plt.savefig('execution_times.png')
plt.show()
