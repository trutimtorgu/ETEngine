

namespace et {
namespace core {


FILE_HANDLE FILE_SYSTEM::Open( const char * pathName, FILE_ACCESS_FLAGS accessFlags, FILE_ACCESS_MODE accessMode )
{

}

bool FILE_SYSTEM::Close( FILE_HANDLE handle )
{

}

bool FILE_SYSTEM::ReadFile( FILE_HANDLE handle, std::string & content )
{

}

bool FILE_SYSTEM::WriteFile( FILE_HANDLE handle, const std::string & content )
{

}

bool FILE_SYSTEM::DeleteFile( const char * pathName )
{

}

bool FILE_SYSTEM::Exists(char const* fileName)
{
	return false;
}


} // namespace core
} // namespace et
