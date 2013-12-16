#import "FileStream.h"
#import "Log.h"
#import "StreamReader.h"
#import "StringEx.h"
#import "vwAlbum.h"
#import "vwMain.h"

@interface MyImageView : UIImageView
{
	id delegate;
	SEL action;
}

-(id)initWithImage:(UIImage*)image title:(NSString*)title delegate:(id)delegate action:(SEL)action;

@end

@implementation MyImageView

-(id)initWithImage:(UIImage*)image title:(NSString*)title delegate:(id)_delegate action:(SEL)_action
{
	if ((self = [super initWithImage:image]))
	{
		[self setUserInteractionEnabled:YES];
		
		delegate = _delegate;
		action = _action;
		
		UILabel* titleLabel = [[[UILabel alloc] initWithFrame:CGRectMake(0.0f, 0.0f, self.bounds.size.width, 20.0f)] autorelease];
		[titleLabel setFont:[UIFont systemFontOfSize:14.0f]];
		[titleLabel setText:title];
		[self addSubview:titleLabel];
	}
	
	return self;
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	[delegate performSelector:action withObject:self.image];
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
}

@end

@implementation vwAlbum

@synthesize scrollView;

-(id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil delegate:(id<AlbumDelegate>)_delegate {
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
        // Custom initialization
		delegate = _delegate;
    }
    return self;
}

- (void)viewDidLoad 
{
    [super viewDidLoad];
	
	float y = 0.0;
	
	int index[] =
	{
		5, 13, 3, 6, 9, 14, 11, 15, 2, -1
	};
	
	const int count = sizeof(index) / sizeof(int);
	
	for (int i = 0; i < count; ++i)
	{
		int idx = index[i];
		
		if (idx < 0)
			continue;
		
		NSString* name = [NSString stringWithFormat:@"album%d", idx];
		NSString* nameTxt = [NSString stringWithFormat:@"album%d.txt", idx];
		NSString* pathTxt = [[NSBundle mainBundle] pathForResource:nameTxt ofType:nil];
		
		LOG_DBG("adding %s", [name cStringUsingEncoding:NSASCIIStringEncoding]);
		
		UIImage* image = [UIImage imageNamed:name];
		
		if (image == nil)
		{
			LOG_WRN("image not found: %s", [name cStringUsingEncoding:NSASCIIStringEncoding]);
			continue;
		}
		
		FileStream stream;
		stream.Open([pathTxt cStringUsingEncoding:NSASCIIStringEncoding], OpenMode_Read);
		StreamReader reader(&stream, false);
		std::vector<std::string> lines = reader.ReadAllLines();
		
		if (lines.size() != 1)
			throw ExceptionVA("image description incomplete");
		
		std::string title = lines[0];
		
		MyImageView* imageView = [[[MyImageView alloc] initWithImage:image title:[NSString stringWithCString:title.c_str() encoding:NSASCIIStringEncoding] delegate:self action:@selector(handleSelect:)] autorelease];
		
		float scale = self.view.bounds.size.width / imageView.bounds.size.width;
		
		[imageView setFrame:CGRectMake(0.0f, y, imageView.bounds.size.width * scale, imageView.bounds.size.height * scale)];
		
		y += imageView.bounds.size.height;
		
		[scrollView addSubview:imageView];
	}
	
	[scrollView setContentSize:CGSizeMake(self.view.bounds.size.width, y)];
}

-(void)handleSelect:(UIImage*)image
{
	[delegate albumImageSelected:image];
}

-(void)handleCancel:(id)sender
{
	[delegate albumImageDismissed];
}

/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

-(void)dealloc 
{
	self.scrollView = nil;
	
    [super dealloc];
}


@end
