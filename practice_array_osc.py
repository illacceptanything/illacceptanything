import pandas as pd

# Read data from Excel file into a DataFrame
excel_file = "py projects\dld.xlsx"
df = pd.read_excel(excel_file)

# Convert DataFrame to a 2D array
arr = df.values.tolist()

# Display values of Images in a tabular form
rows = len(arr)
cols = len(arr[0])
for i in range(rows):
    for j in range(cols):
        print(arr[i][j], end=" ", sep="   ")
    print()

k = 0
while k < rows:
    for l in range(k + 1, rows):
        count = 0
        for m in range(cols):
            if arr[k][m] == arr[l][m]:
                count += 1
        if count == cols:  # Check if all values in a row are the same
            print((k + 1), "is same as", (l + 1))
    k += 1
