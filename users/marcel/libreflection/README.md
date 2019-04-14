# libreflection
libreflection is a tiny type reflection library for c++. It's application is to provide type reflection information for structured types, basic data types, and arrays (std::vector).

libreflection is optimized for fast compile times. It uses a minimum of c++ template magic to keep the compiler happy. Some magic is however needed, but it shouldn't push the compiler to its limits to such an extent that it will bloat compile times.

libreflection takes the approach to explicitly build a type database. There is no global type database. The user of the library has to take care to explicitly create a type database and fill it with type information. The rationale behind this is that there are use cases where it is desirable to expose only a subset of data for (de)serialization purposes.

## Creating a type database

```cpp
struct ChromaticSomething // aka a rainbow
{
	bool chromatic = true;
	std::vector<int> hues;
	float density = 1.f;
};

TypeDB typeDB;

typeDB.addPlain<bool>("bool", kDataType_Bool);
typeDB.addPlain<int>("int", kDataType_Int);
typeDB.addPlain<float>("float", kDataType_Float);
	
typeDB.addStructured<ChromaticSomething>("ChromaticSomething")
	.add("chromatic", &ChromaticSomething::chromatic)
	.add("hues", &ChromaticSomething::hues)
	.add("density", &ChromaticSomething::density);
```

## Iterating a structured data type
Below is an example of iterating a structured type's fields and dumping its contents. Note that this example doesn't handle types recursively. It only dumps plain data type fields of the type being inspected. For a more elaborate example, see the example app 'libreflection-020-structured', which illustrated how to traverse structured data recursively.

```cpp
ChromaticSomething something;
something.chromatic = false;
something.density = .76f;
	
auto * type = typeDB.findType<ChromaticSomething>();
	
if (type->isStructured)
{
	auto * structured_type = static_cast<const StructuredType*>(type);
	
	printf("type: %s\n", structured_type->typeName);
	
	for (auto * member = structured_type->members_head; member != nullptr; member = member->next)
	{
		printf("\tmember: %s\n", member->name);

		if (member->isVector)
			continue; // ignore arrays for now

		auto * member_scalar = static_cast<const Member_Scalar*>(member);
		void * member_object = member_scalar->scalar_access(&something);
		
		auto * member_type = typeDB.findType(member_scalar->typeIndex);
		
		if (member_type->isStructured)
			continue; // ignore structured types within structured types for now
		
		auto * plain_type = static_cast<const PlainType*>(member_type);
		
		switch (plain_type->dataType)
		{
		case kDataType_Bool:
			{
				const bool value = plain_type->access<bool>(member_object);
				printf("\t\tboolean %s\n", value ? "true" : "false");
			}
			break;
			
		case kDataType_Int:
			{
				const int value = plain_type->access<int>(member_object);
				printf("\t\tint %d\n", value);
			}
			break;
			
		case kDataType_Float:
			{
				const float value = plain_type->access<float>(member_object);
				printf("\t\tfloat %g\n", value);
			}
			break;
			
		default:
			break;
		}
	}
}
```

## Array access (std::vector)
Array members are added to structured types in the same way as any other type.

```cpp
	typeDB.addStructured<Dataset>("Dataset")
		.add("subsets", &Dataset::subsets);
```

However, where single valued members add a Member_Scalar to the type database, vector types add a Member_Vector<T>, which derives from Member_VectorInterface. Member_VectorInterface provides the interface for working with vector types.

```cpp
struct Member_VectorInterface : Member
{
	virtual std::type_index vector_type() const = 0;
	virtual size_t vector_size(const void * object) const = 0;
	virtual void vector_resize(void * object, const size_t size) const = 0;
	virtual void * vector_access(void * object, const size_t index) const = 0;
	virtual void vector_swap(void * object, const size_t index1, const size_t index2) const = 0;
	
	const void * vector_access(const void * object, const size_t index) const
	{
		return vector_access((void*)object, index);
	}
};
```

The values inside a vector can be iterate as follows.

```cpp
if (member->isVector)
{
	auto * member_vector = static_cast<const 	Member_VectorInterface*>(member);
				
	auto * vector_type = typeDB.findType(member_vector->vector_type());
	
	// a nullptr type could happen if the type database is incomplete
	assert(vector_type != nullptr);
	if (vector_type == nullptr)
		continue;
	
	const size_t vector_size = member_vector->vector_size(object);
	
	printf("member: %s [%zu]\n", member->name, vector_size);
	
	for (size_t i = 0; i < vector_size; ++i)
	{
		auto * vector_object = member_vector->vector_access(object, i);
		
		// printStructuredObject will print the contents of 
		// vector_object (the i-th array element),
		// of type vector_type
		printStructuredObject(typeDB, vector_type, vector_object);
	}
}			
```


## More examples
libreflection is bundle with a number of examples. Use chibi to build all libreflection targets and you will find them in your project file or solution.

List of examples:

- libreflection-000-typedb: Shows how to populate a type database.
- libreflection-010-reflection: Shows how to do basic reflection to print the contents of a structured type.
- libreflection-020-structured: Shows how to do recursive reflection to print the contents of structured types and arrays.
- libreflection-100-flags: Shows how to add flags to members.