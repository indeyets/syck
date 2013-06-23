//
//  YAMLCategories.m
//  YAML
//
//  Created by William Thimbleby on Sat Sep 25 2004.
//  Copyright (c) 2004 William Thimbleby. All rights reserved.
//

#import "YAMLCategories.h"
#import "GSNSDataExtensions.h"

BOOL yamlClass(id object)
{
	if([object isKindOfClass:[NSArray class]])
		return YES;
	if([object isKindOfClass:[NSDictionary class]])
		return YES;
	if([object isKindOfClass:[NSString class]])
		return YES;
	if([object isKindOfClass:[NSNumber class]])
		return YES;
	if([object isKindOfClass:[NSData class]])
		return YES;
	return NO;
}

@implementation YAMLWrapper

+ (id)wrapperWithData:(id)d tag:(Class)cn
{
	return [[[YAMLWrapper alloc] initWrapperWithData:d tag:cn] autorelease];
}

- (id)initWrapperWithData:(id)d tag:(Class)cn
{
	self = [super init];
	data = [d retain];
	tag = cn;
	return self;
}

- (void)dealloc
{
	[data release];
    [super dealloc];
}

- (id)copyWithZone:(NSZone *)zone
{
	YAMLWrapper *w = [YAMLWrapper allocWithZone:zone];
	
	[w initWrapperWithData:data tag:tag];
	
	return [w autorelease];
}

- (id)data
{
	return data;
}

- (Class)tag
{
	return tag;
}

- (id)yamlParse
{
	//NSLog(@"%@-%@",tag,data);
	return [tag performSelector:@selector(objectWithYAML:) withObject:data];
}

@end

#pragma mark -

@implementation NSString (YAMLAdditions)

+ (id)yamlStringWithUTF8String:(const char *)bytes length:(unsigned)length
{
	NSString *str = [[NSString alloc] initWithBytes:bytes length:length encoding:NSUTF8StringEncoding];
	
	return [str autorelease];
}

-(int) yamlIndent
{
	int i;
	//calculate the indent
	i = 0;
	while(i < [self length] && [self characterAtIndex:i] == ' ')
		i = i+1;
	
	return i;
}

-(NSString*) yamlIndented:(int)indent
{
	NSRange				lineRange;
	int					i = [self length]-1;
	NSMutableString		*indented = [self mutableCopy];	
	NSString            *stringIndent = [@"" stringByPaddingToLength:indent withString:@" " startingAtIndex:0];
	
	while(i > 0)
	{
		lineRange = [indented lineRangeForRange:NSMakeRange(i,0)];
		
		[indented insertString:stringIndent atIndex:lineRange.location];
		
		i = lineRange.location - 1;
	}
	
	return [indented autorelease];
}

-(NSString*)yamlDescriptionWithIndent:(int)indent
{
	NSRange		lineRange;
	
	lineRange = [self lineRangeForRange:NSMakeRange(0,0)];
	
	//if no line breaks in string
	if(lineRange.length >= [self length])
		return [NSString stringWithFormat:@"\"%@\"", [self stringByReplacingOccurrencesOfString:@"\"" withString:@"\\\""]];
	
	return [NSString stringWithFormat:@"|-\n%@", [self yamlIndented:indent]];
}

- (id)toYAML
{
	return self;
}

@end

@implementation NSArray (YAMLAdditions)

-(NSArray*) yamlData 
{    
	NSMutableArray *array = [NSMutableArray arrayWithCapacity:[self count]];
	
	NSEnumerator *enumerator = [self objectEnumerator];
    id object;
	while (object = [enumerator nextObject]) {
		[array addObject:[object yamlData]];
	}
	
	return array;
}

- (NSArray*) yamlParse
{
	return [self yamlCollectWithSelector:@selector(yamlParse)];
}

-(NSString*) yamlDescriptionWithIndent:(int)indent
{
    indent -= 2;
	NSEnumerator		*enumerator = [self objectEnumerator];
	id					anObject, last = [self lastObject];
	NSMutableString		*description = [NSMutableString stringWithString:@"\n"];
	char				*strIndent = malloc(indent+1);
	
	if([self count] == 0)
		return @"[]";
	
	memset(strIndent, ' ', indent);
	strIndent[indent] = 0;
			
	while (anObject = [enumerator nextObject])
	{
		NSString	*tag;
		
		if(yamlClass(anObject))
			tag = @"";
		else 
			tag = [NSString stringWithFormat:@"!!%@ ", NSStringFromClass([anObject class])];
		
		anObject = [anObject toYAML];
		
		[description appendFormat:@"%s- %@%@%s", strIndent, tag,
			[anObject yamlDescriptionWithIndent:indent+2], anObject == last? "" : "\n"];
	}
	
	free(strIndent);
	
	return description;
}

- (id)toYAML
{
	return self;
}

- (NSArray*)yamlCollectWithSelector:(SEL)aSelector withObject:(id)anObject
{
	NSMutableArray *array = [NSMutableArray arrayWithCapacity:[self count]];
    NSEnumerator *enumerator = [self objectEnumerator];
    id object;
    while((object = [enumerator nextObject])) {
        [array addObject:[object performSelector:aSelector withObject:anObject]];
    }
    return array;
}

- (NSArray*)yamlCollectWithSelector:(SEL)aSelector
{
	NSMutableArray *array = [NSMutableArray arrayWithCapacity:[self count]];
    NSEnumerator *enumerator = [self objectEnumerator];
    id object;
    while((object = [enumerator nextObject])) {
        [array addObject:[object performSelector:aSelector]];
    }
    return array;
}

@end

@implementation NSDictionary (YAMLAdditions)

-(NSDictionary*) yamlData
{
	NSMutableDictionary	*dict = [NSMutableDictionary dictionaryWithCapacity:[self count]];
	
	NSEnumerator *enumerator = [self keyEnumerator];
    id key;
	while (key = [enumerator nextObject]) {
		[dict setObject:[[self objectForKey:key] yamlData] forKey:key];
	}
	
	return dict;
}

- (NSDictionary*)yamlParse
{
	return [self yamlCollectWithSelector:@selector(yamlParse)];
}

-(NSString*) yamlDescriptionWithIndent:(int)indent
{
    if([self count] == 0)
		return @"{}";

	NSMutableString *description = [NSMutableString string];
	NSString *strIndent = nil;
    if(indent > 0) {
        [@"" stringByPaddingToLength:indent withString:@" " startingAtIndex:0];   
    }
    
	//output
    NSEnumerator *enumerator = [self keyEnumerator];
    NSString *key;
	while((key = [enumerator nextObject])) {
        id object = [self objectForKey:key];

        [description appendString:@"\n"];
        if(strIndent) {
            [description appendString:strIndent];
        }

        [description appendFormat:@"%@: ", key];
        if(!yamlClass(object)) {
            [description appendFormat:@"!!%@ ", NSStringFromClass([object class])];
        }
        [description appendString:[[object toYAML] yamlDescriptionWithIndent:indent+2]];
	}
	
	return description;
}

- (id)toYAML
{
	return self;
}

- (NSDictionary*)yamlCollectWithSelector:(SEL)aSelector withObject:(id)anObject
{
	NSMutableDictionary  *dict = [NSMutableDictionary dictionaryWithCapacity:[self count]];
    
    NSEnumerator *keyEnumerator = [self keyEnumerator];
    id key;
    while((key = [keyEnumerator nextObject])) {
        [dict setObject: [[self objectForKey:key] performSelector:aSelector withObject:anObject]
                 forKey: key];
    }
    return dict;
}

- (NSDictionary*)yamlCollectWithSelector:(SEL)aSelector
{
	NSMutableDictionary  *dict = [NSMutableDictionary dictionaryWithCapacity:[self count]];
    
    NSEnumerator *keyEnumerator = [self keyEnumerator];
    id key;
    while((key = [keyEnumerator nextObject])) {
        [dict setObject: [[self objectForKey:key] performSelector:aSelector]
                 forKey: key];
    }
    return dict;
}

@end

@implementation NSObject (YAMLAdditions)

-(NSString*) yamlDescriptionWithIndent:(int)indent
{
	return [self toYAML];
}

- (void)yamlPerformSelector:(SEL)sel withEachObjectInArray:(NSArray *)array {
    NSEnumerator *enumerator = [array objectEnumerator];
    id object;
    while((object = [enumerator nextObject])) {
        [self performSelector:sel withObject:object];
    }
}

- (void)yamlPerformSelector:(SEL)sel withEachObjectInSet:(NSSet *)set {
    [self yamlPerformSelector:sel withEachObjectInArray:[set allObjects]];
}

-(NSString*) yamlDescription
{
	return [self yamlDescriptionWithIndent:0];
}

- (id)yamlParse
{
	return self;
}

-(id) yamlData
{
	if(!yamlClass(self))
        return [YAMLWrapper wrapperWithData:[self toYAML] tag:[self class]];
	else
		return [self toYAML];
}

-(id) toYAML
{
	return [self description];
}

@end

@implementation NSData (YAMLAdditions) 
-(id) yamlDescriptionWithIndent:(int)indent
{
    return [[@"!binary |\n" stringByAppendingString:[self base64EncodingWithLineLength:72]] yamlIndented:indent];
}

-(id) toYAML
{
	return self;
}
@end