import random
import json

common_index_num = 3


def generate_sparse_vectors(size, num_elements_doc, num_elements_query):
    if num_elements_doc + num_elements_query - 2 > size:
        raise ValueError(
            "Total number of elements in doc and query cannot exceed vector size plus 2."
        )

    # Generate common indices
    common_indices = random.sample(range(size), common_index_num)

    # Generate remaining indices for doc and query
    remaining_indices_doc = sorted(
        random.sample(
            [i for i in range(size) if i not in common_indices], num_elements_doc - 2
        )
    )
    remaining_indices_query = sorted(
        random.sample(
            [i for i in range(size) if i not in common_indices], num_elements_query - 2
        )
    )

    # Combine indices
    indices_doc = sorted(remaining_indices_doc + common_indices)
    query_indices = sorted(remaining_indices_query + common_indices)

    # Generate random values
    values_doc = [random.uniform(0, 1) for _ in range(num_elements_doc)]
    query_values = [random.uniform(0, 1) for _ in range(num_elements_query)]

    return indices_doc, values_doc, query_indices, query_values


def generate_sparse_vectors2(doc_number, vector_dim, num_elements_query):
    # Generate common indices
    common_indices = random.sample(range(vector_dim), common_index_num)

    doc_indices_vector = []
    doc_values_vector = []

    for i in range(doc_number):
        num_elements_doc = random.randint(4, 10)
        # Generate remaining indices for doc and query
        remaining_indices_doc = sorted(
            random.sample(
                [i for i in range(vector_dim) if i not in common_indices],
                num_elements_doc - len(common_indices),
            )
        )
        indices_doc = sorted(remaining_indices_doc + common_indices)
        values_doc = [random.uniform(0, 1) for _ in range(num_elements_doc)]

        doc_indices_vector.append(indices_doc)
        doc_values_vector.append(values_doc)

    remaining_indices_query = sorted(
        random.sample(
            [i for i in range(vector_dim) if i not in common_indices],
            num_elements_query - len(common_indices),
        )
    )
    # Combine indices
    query_indices = sorted(remaining_indices_query + common_indices)

    # Generate random values
    query_values = [random.uniform(0, 1) for _ in range(num_elements_query)]

    return doc_indices_vector, doc_values_vector, query_indices, query_values


def save_docs_to_file(filename, doc_indices_vector, doc_values_vector):
    with open(filename, "w") as file:
        data = {
            "doc_count": len(doc_indices_vector),
            "indices": doc_indices_vector,
            "values": doc_values_vector,
        }
        json.dump(data, file, indent=4)


def save_query_to_file(filename, indices, values):
    with open(filename, "w") as file:
        data = {"indices": indices, "values": values}
        json.dump(data, file, indent=4)


vector_dim = 20005  # Size of the sparse vector
doc_number = 4  # Number of docs
num_elements_doc = 8  # Number of non-zero elements in doc
num_elements_query = 6  # Number of non-zero elements in query

doc_indices_vector, doc_values_vector, query_indices, query_values = (
    generate_sparse_vectors2(doc_number, vector_dim, num_elements_query)
)

# print("Doc Indices:", indices_doc)
# print("Doc Values:", values_doc)
# print("Query Indices:", query_indices)
# print("Query Values:", query_values)

# Save results to files
save_docs_to_file("docs_sparse_vector.json", doc_indices_vector, doc_values_vector)
save_query_to_file("query_sparse_vector.json", query_indices, query_values)


def sparse_dot_product(index_a, value_a, index_b, value_b):
    # 인덱스와 값의 쌍을 딕셔너리로 변환
    dict_a = dict(zip(index_a, value_a))
    dict_b = dict(zip(index_b, value_b))

    # 두 벡터의 dot product 계산
    dot_product = 0
    for key in dict_a:
        if key in dict_b:
            dot_product += dict_a[key] * dict_b[key]

    return dot_product


result_list = []
for i in range(doc_number):
    result = sparse_dot_product(
        doc_indices_vector[i], doc_values_vector[i], query_indices, query_values
    )
    result_list.append(result)

print("Dot Product:", result_list)

with open("result.json", "w") as file:
    data = {"result": result_list}
    json.dump(data, file, indent=4)
