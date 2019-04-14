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

## More examples

todo : add libreflection examples