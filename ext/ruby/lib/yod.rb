#												vim:sw=4:ts=4
# $Id$
#
# YOD: Yaml Ok Documentation
#
require 'yaml'

#
# Hi.  Yod is... well... something.  Okay, look.
# It's a personal project for building documentation.
# It's also meant to be an example project.  To give
# a more complete example of YAML's use.
#
module Yod

	VERSION = '0.2'
	SECTION_GIF =<<EOF
R0lGODlhCQAMAMYAAERFP0NEPj0+OEFCPEhJQ0pLRTw9N1NUTlZXUVhZU1xd
V1laVDY3MVFSTWRlYIeIg5WWkZOUj19gWz9AO1hZVG9wa6Kjnri5tLGyrbCx
rLS1sD0+OVVWUW1uaaanor/Au8DBvHN0bzk6NVlaVXBxbKeoo76/urq7tsPE
v3d4c0BBPExLSWFgXpGQjqOioJybmWdmZDg3NURDQVdWVIB/fYyLiYKBf4OC
gFtaWDMyMD49O0dGRGBfXXJxb25ta0pJRzIxLzk4Nj08Ok1MSlVUUlJRT1RT
UUVEQi4tKygkJS8rLDIuLy4qKy0pKjEtLjUxMiEdHicjJCwoKSsnKCklJjQw
Mf//////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
/////////////////////////////////////////////////yH+FUNyZWF0
ZWQgd2l0aCBUaGUgR0lNUAAsAAAAAAkADAAAB22AAAABAgMEAQUEBgAHCAkK
CwAMDQ4PEBGYEhMUFRYXGBkaFRscHR4fFxcgISIjJCUmFycoKSorLC0uLy8u
MDEyMzQ1NjQ3ODk6Ozw9PT4+P0BBQkMzREVGR0hJSUpLTE1OT1BRSVJTSVRM
VVGBADs=
EOF

	TEXT_CSS =<<EOF
.sectionhead {
	background-color : #667788;
	color: white;
	font-family : Verdana, Geneva, Arial, Helvetica, sans-serif;
	padding: 5px;
	border-bottom : 1. solid;
	font-weight: bold;
}
.contentshead {
	background-color : black;
	color: white;
	font-family : Verdana, Geneva, Arial, Helvetica, sans-serif;
	padding: 5px;
	border-bottom : 1. solid;
	font-weight: bold;
    width: 130%;
}
.quickhead {
	background-color : white;
	color: black;
	font-family : Verdana, Geneva, Arial, Helvetica, sans-serif;
	padding: 10px 5px 5px 5px;
	border-bottom : 1. solid;
	font-weight: bold;
}
.functionhead {
	background-color : #333344;
	color: white;
	font-family : Verdana, Geneva, Arial, Helvetica, sans-serif;
	padding: 5px;
	border-bottom : 1. solid;
	font-weight: bold;
}
.indent1       { margin-left:12pt; margin-right:12pt;}
.indent2       { margin-left:24pt;margin-right:24pt; }
.indent3       { margin-left:36pt; margin-right:24pt;}
Th {
	font-style : italic;
	font-size : x-small;
	font-weight : normal;
	background-color : #DCDCDC;
}


BLOCKQUOTE {
	margin-left : 30;
	margin-right : 30;
	color: #112233;
}

.function {
	font-weight : bold;
	font-size : larger;
	margin-top : 8;
	margin-left : 12pt;
	margin-bottom: 5px;
}

.welcomehead {
	background-color : #333333;
	color: white;
	width : 100%;
	padding : 5;
	border-bottom : 1 solid;
	text-align : left;
	vertical-align : middle;
	font-family : Verdana, Geneva, Arial, Helvetica, sans-serif;
	font-size : medium;
	font-weight : bold;
}

p
{
	margin-left : 15;
	margin-right : 15;
}

.summarytable
{
	margin-left: 15;
	margin-right: 15;
	border-width: 1px;
	border-color: ACACAC;
	border-width: solid;
}
UL
{
	margin-top: 0px;
}
TD,body
{
	margin-left : 0;
	margin-right : 0;
	margin-top : 0;
	font-family : Verdana, Geneva, Arial, Helvetica, sans-serif;
	font-size : x-small;
	vertical-align : top;
}

TD
{
	padding-left:12pt;
}

.codeblock
{
	margin-left: 20;
	margin-right: 20;
	background-color : #EFEFEF;
	font-family : "Courier New", Courier, monospace;;
	padding : 4;
	font-size: 10pt;
}
DL {
	margin-left : 12pt;
	margin-right : 12pt;
	margin-top: 3;
}
DT
{
	margin-left:12pt; margin-right:12pt;	
	font-style : italic;
}

.keyword
{
	color: #0000FF;
}

.func
{
	color: #FF0000;
}
EOF
    LEFT = 10
    CENTER = 20
    RIGHT = 30

	class Error < StandardError; end

	class Document

		attr_accessor :table_of_contents, :contents, :title, :author, :version

		def initialize( data )
			@contents = {}
			data.each_pair { |key, val|
				if key == "Table of Contents"
					@table_of_contents = val 
				elsif key == "Title"
					@title = val
                elsif key == "Author"
                    @author = val
                elsif key == "Version"
                    @version = val
				elsif val.is_a?( Yod::DocElement )
					val.title = key
					@contents[key] = val
				else
					raise Yod::Error, "Invalid Document node #{key} is not a Page or Group."
				end
			}
		end

		def pages
			@table_of_contents.collect { |x| 
				@contents[x].pages 
			}.flatten
		end

		def index
			index_all = {}
			@table_of_contents.each { |x| 
				index_all.update( @contents[x].index ) if @contents[x].respond_to? :index
			}
			index_all.sort
		end

        def to_man( *args )
			page = args.shift
			case page
				when :Make
                    output, name = args

					File.open( output, "w" ) { |f|
						f.write( <<HEADER )
.TH #{title.upcase} 3 "#{Time.now.strftime( '%d %F %Y' )}"
.SH NAME
#{@title}
HEADER
                        @table_of_contents.each_with_index { |x, i| 
                            f.write( @contents[x].to_man( :Page, 0 ) )
                        }
					}

            end
        end

        def to_pdf( *args )
			page = args.shift
			case page
				when :Make
					output = args.shift
                    require "ClibPDF"

                    pdf = ClibPDF::PDF.open( 0 )
                    pdf.enableCompression( ClibPDF::YES )
                    pdf.init
                    pdf.setTitle( @title )
                    pdf.pageInit(1, ClibPDF::PORTRAIT, ClibPDF::LETTER, ClibPDF::LETTER)

                    titleBase = 6.5
                    pdf.beginText( 0 )
                    pdf.setFont( "NewCenturySchlbk-Roman", "MacRomanEncoding", 20.0)
                    pdf.textAligned( 4.4, titleBase, 0.0, ClibPDF::TEXTPOS_MM, @title )
                    pdf.endText

                    pdf.setgray(0.0)
                    pdf.setlinewidth(0.2)
                    pdf.moveto(1.0, titleBase - 0.5)
                    pdf.lineto(7.8, titleBase - 0.5)
                    pdf.stroke

                    if @author
                        pdf.beginText( 0 )
                        pdf.setFont( "NewCenturySchlbk-Roman", "MacRomanEncoding", 14.0)
                        pdf.textAligned( 4.4, titleBase - 0.8, 0.0, ClibPDF::TEXTPOS_MM, "by #{@author}" )
                        pdf.endText
                    end

                    if @version
                        pdf.beginText( 0 )
                        pdf.setFont( "NewCenturySchlbk-Roman", "MacRomanEncoding", 10.0)
                        pdf.textAligned( 4.4, titleBase - 1.2, 0.0, ClibPDF::TEXTPOS_MM, "[Version #{@version}]" )
                        pdf.endText
                    end

                    pdf.pageInit(2, ClibPDF::PORTRAIT, ClibPDF::LETTER, ClibPDF::LETTER)
                    pdf.beginText( 0 )
                    pdf.setFont( "NewCenturySchlbk-Roman", "MacRomanEncoding", 14.0)
                    pdf.text(1.0, 10.4, 0.0, "Table of Contents")
                    pdf.endText

                    pdf.setgray(0.0)
                    pdf.setlinewidth(0.2)
                    pdf.moveto(1.0, 10.2)
                    pdf.lineto(7.8, 10.2)
                    pdf.stroke

                    pdf.beginText( 0 )
                    pdf.setFont( "NewCenturySchlbk-Roman", "MacRomanEncoding", 10.0)
                    pdf.setTextLeading(12.0)
                    pdf.setTextPosition(1.0, 10.0)
                    self.to_pdf( :TableOfContents, pdf )
                    pdf.endText

                    def pdf.initPara( title )
                        @guts = {
                            :page => 2,
                            :y => 0.0,
                            :title => title,
                            :sections => []
                        }
                    end
                    def pdf.textPara( fontName, fontSize, fontAlign, indent, ybump, textchunk, force_page = false )
                        if force_page
                            @guts[:y] = 0.0
                        end
                        begint = false
                        textchunk.split( /\n/ ).each do |text|
                            j = 0
                            j_p = 0
                            line_width = ( 6.6 - ( indent * 2.0 ) ) * ClibPDF::INCH
                            self.setFont( fontName, "MacRomanEncoding", fontSize )
                            while j = text.index( /\s/, j ) or text.length.nonzero?
                                unless j
                                    j = -1
                                    j_p = -1
                                end
                                if j < 0 or self.stringWidth( text[ 0..j-1 ] ) > line_width

                                    if @guts[:y] < 0.6   # Start a new page
                                        if begint
                                            self.endText
                                            begint = false
                                        end
                                        @guts[:page] += 1
                                        @guts[:y] = 10.2
                                        self.pageInit( @guts[:page], ClibPDF::PORTRAIT, ClibPDF::LETTER, ClibPDF::LETTER)

                                        self.beginText( 0 )
                                        self.setFont( "NewCenturySchlbk-Roman", "MacRomanEncoding", 10.0)
                                        self.text(1.0, 0.2, 0.0, @guts[:title])
                                        self.textAligned( 7.8, 0.2, 0.0, ClibPDF::TEXTPOS_MR, "Page #{@guts[:page]}" )
                                        self.endText

                                        self.setgray(0.0)
                                        self.setlinewidth(0.2)
                                        self.moveto(1.0, 0.4)
                                        self.lineto(7.8, 0.4)
                                        self.stroke
                                    end

                                    #
                                    # Set the font
                                    #
                                    unless begint
                                        self.beginText( 0 )
                                        self.setFont( fontName, "MacRomanEncoding", fontSize )
                                        begint = true
                                    end

                                    line = text[ 0..j_p ]
                                    # self.text( 1.0 + indent, @guts[:y], 0.0, line )
                                    align = ClibPDF::TEXTPOS_ML
                                    origin = 1.0 + indent
                                    if fontAlign == RIGHT
                                        align = ClibPDF::TEXTPOS_MR
                                        origin = 7.8 - indent
                                    elsif fontAlign == CENTER
                                        align = ClibPDF::TEXTPOS_MM
                                        origin = 4.4
                                    end
                                    self.textAligned( origin, @guts[:y], 0.0, align, line )
                                    if j_p > 0
                                        text = text[ j_p + 2..-1 ]
                                    else
                                        text = ""
                                    end
                                    j = 0
                                    j_p = 0
                                    @guts[:y] -= ybump
                                    next
                                end
                                j_p = j - 1
                                j += 1
                            end
                        end
                        self.newline
                        if begint
                            self.endText
                        end
                    end
                    def pdf.newline
                        @guts[:y] -= 0.15
                    end
                    pdf.initPara( @title )

                    @table_of_contents.each_with_index { |x, i| 
                        @contents[x].to_pdf( :Page, pdf, [ i + 1 ] ) 
                    }

                    pdf.finalizeAll
                    pdf.savePDFmemoryStreamToFile( output )
                    pdf.close()

				when :TableOfContents
                    pdf = args.shift
                    @table_of_contents.each_with_index { |x, i| 
                        @contents[x].to_pdf( :TableOfContents, pdf, i + 1 ) 
                    }
            end
        end

		def to_chm( *args )
			page = args.shift
			case page
				when :MakeAll
					output, prefix = args
					Yod.multi_mkdir( output, 0755 )

					# Write the INI
					File.open( File.join( output, "#{prefix}.hhp" ), "w" ) { |f|
						f.write( self.to_chm( :IniFile, prefix ) )
					}

					# Write the bullet GIF
					File.open( File.join( output, "section.gif" ), "w" ) { |f|
						f.write( Yod::SECTION_GIF.unpack( "m*" )[0] )
					}

					# Write the CSS
					File.open( File.join( output, "global.css" ), "w" ) { |f|
						f.write( Yod::TEXT_CSS )
					}

					# Write the Index
					File.open( File.join( output, "Index.hhk" ), "w" ) { |f|
						f.write( self.to_chm( :Index ) )
					}

					# Write the TOC
					File.open( File.join( output, "Table of Contents.hhc" ), "w" ) { |f|
						f.write( self.to_chm( :TableOfContents ) )
					}

					# Write each page of the manual
					self.pages.each { |p|
						Yod.multi_mkdir( output + File::SEPARATOR +
							p.html_file_name.split( File::SEPARATOR )[0], 0755 )
						File.open( File.join( output, p.html_file_name ), "w" ) { |f|
							f.write( p.to_chm( :Html ) )
						}
					}

				when :IniFile
					prefix = args.shift
					page_list = self.pages.collect { |p| p.html_file_name }
					return <<EOF
[OPTIONS]
Binary Index=No
Binary TOC=Yes
Compatibility=1.1 or later
Compiled file=#{prefix}.chm
Contents file=Table of Contents.hhc
Create CHI file=Yes
Default topic=#{page_list.first}
Display compile progress=No
Index file=Index.hhk
Language=0x409 English (United States)
Title=#{@title}


[FILES]
#{page_list.join( "\n" )}

[INFOTYPES]
Category:Bitmaps
CategoryDesc: 
EOF
				when :Index
					index_content = self.index.collect { |item| <<EOF
	<LI> <OBJECT type="text/sitemap">
		<param name="Name" value="#{item[0]}">
		<param name="Local" value="#{item[1]}">
		</OBJECT>
EOF
					}
					return <<EOF
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<HTML>
<HEAD>
<meta name="GENERATOR" content="Microsoft&reg; HTML Help Workshop 4.1">
<!-- Sitemap 1.0 -->
</HEAD><BODY>
<UL>
#{index_content}
</UL>
</BODY></HTML>
EOF
				when :TableOfContents
					return <<EOF
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<HTML>
<HEAD>
<meta name="GENERATOR" content="Microsoft&reg; HTML Help Workshop 4.1">
<!-- Sitemap 1.0 -->
</HEAD><BODY>
<OBJECT type="text/site properties">
<param name="Window Styles" value="0x800025">
</OBJECT>
 <UL>
 #{@table_of_contents.collect { |x| @contents[x].to_chm( :TableOfContents ) }}
 </UL>
</BODY></HTML>
EOF
			end
		end

		def to_html( *args )
			page = args.shift
			case page
				when :MakeAll
					output = args.shift
					Yod.multi_mkdir( output, 0755 )

					# Write the bullet GIF
					File.open( File.join( output, "section.gif" ), "w" ) { |f|
						f.write( Yod::SECTION_GIF.unpack( "m*" )[0] )
					}

					# Write the CSS
					File.open( File.join( output, "global.css" ), "w" ) { |f|
						f.write( Yod::TEXT_CSS )
					}

					# Write the Index frame
					File.open( File.join( output, "index.html" ), "w" ) { |f|
						f.write( self.to_html( :Frame ) )
					}

					# Write the TOC
					File.open( File.join( output, "contents.html" ), "w" ) { |f|
						f.write( self.to_html( :TableOfContents ) )
					}

					# Write each page of the manual
					self.pages.each { |p|
						Yod.multi_mkdir( output + File::SEPARATOR +
							p.html_file_name.split( File::SEPARATOR )[0], 0755 )
						File.open( File.join( output, p.html_file_name ), "w" ) { |f|
							f.write( p.to_html( :Html ) )
						}
					}

				when :Frame
					prefix = args.shift
					page_list = self.pages.collect { |p| p.html_file_name }
					return <<EOF
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Frameset//EN"
	"http://www.w3.org/TR/html4/frameset.dtd">
<HTML>
<HEAD>
	<TITLE>#{@title}</TITLE>
</HEAD>
<FRAMESET cols="200,*">
	<FRAME src="contents.html">
	<FRAME name="docview" src="#{page_list.first}">
</FRAMESET>
</HTML>
EOF

				when :TableOfContents
					return <<EOF
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<HTML>
<HEAD>
    <meta name="GENERATOR" content="Yod #{Yod::VERSION}">
	<link href="global.css" type="text/css" rel="stylesheet">
</HEAD><BODY>
<div class="contentshead">CONTENTS</div>
<p>
#{@table_of_contents.collect { |x| @contents[x].to_html( :TableOfContents ) }}
</p>
</BODY></HTML>
EOF
			end
		end
	end

	class DocElement; attr_accessor :title, :elements; def pages; self; end; end

	class Group < DocElement
		def initialize( data )
			@elements = []
			data.each { |ele|
				name, ele = ele.to_a[0]
				if ele.is_a?( Yod::DocElement ) 
					ele.title = name
					@elements << ele
				else
					raise Yod::Error, "Invalid node of type #{ele.class} in Group: " + ele.inspect
				end
			}
		end

		def pages
			self.elements.collect { |e| 
				e.pages
			}
		end

		def index
			index_all = {}
			self.elements.each { |e| 
				index_all.update( e.index ) if e.respond_to? :index
			}
			index_all
		end

        def to_man( *args )
            page = args.shift
			case page
                when :Page
                    depth = args.shift
                    if depth.zero?
                        str = ".SH #{self.title}\n"
                    else
                        str = ".Sh #{self.title}\n"
                    end
	                self.elements.each { |e| 
                        str += e.to_man( :Page, depth + 1 ) 
                    }
                    str
            end
        end

		def to_pdf( *args )
            page = args.shift
			case page
				when :TableOfContents
                    pdf, idx = args
                    pdf.textCRLFshow( "#{idx}. #{self.title}" )
	                self.elements.each_with_index { |e, i| 
                        e.to_pdf( :TableOfContents, pdf, "#{idx}.#{i+1}" ) 
                    }
                when :Page
                    pdf, idx = args
                    size = 22.0 - ( idx.length * 2.0 )
                    if size < 14.0
                        size = 14.0
                    end
                    pdf.newline
                    pdf.textPara( "NewCenturySchlbk-Roman", size, LEFT, 0.0, 0.4, 
                        "#{idx.join('.')}. #{self.title}", idx.length == 1 ) 
	                self.elements.each_with_index { |e, i| 
                        e.to_pdf( :Page, pdf, [ idx ] + [ i + 1 ] ) 
                    }
			end
		end

		def to_chm( page )
			case page
				when :TableOfContents
					return <<EOF
<LI> <OBJECT type="text/sitemap">
     <param name="Name" value="#{self.title}">
     </OBJECT>
     <UL>
	 #{self.elements.collect { |e| e.to_chm( :TableOfContents ) }}
     </UL>
EOF
			end
		end

		def to_html( page )
			case page
				when :TableOfContents
					return <<EOF
<LI> <B>#{self.title}</B>
     <UL>
	 #{self.elements.collect { |e| e.to_html( :TableOfContents ) }}
     </UL>
EOF
			end
        end
	end

	class Page < DocElement
		def initialize( data )
			@elements = []
			data.each { |ele|
				if ele.is_a?( Yod::PageElement )
					@elements << ele
				else
					raise Yod::Error, "Invalid node of type #{ele.class} in Page: " + ele.inspect
				end
			}
		end

		def html_file_name
			"page" + File::SEPARATOR + @title.downcase.gsub( /[^A-Za-z0-9]/, '_' ) + ".htm"
		end

		def index
			index_all = { @title => self.html_file_name }
		end

        def to_man( *args )
            page = args.shift
			case page
                when :Page
                    depth = args.shift
                    if depth.zero?
                        str = ".SH #{self.title}\n"
                    else
                        str = ".Sh #{self.title}\n"
                    end
                    self.elements.collect { |e| 
                        str += e.to_man 
                    }
                    str
            end
        end

		def to_pdf( *args )
            page = args.shift
			case page
				when :TableOfContents
                    pdf, idx = args
                    pdf.textCRLFshow( "#{idx}. #{self.title}" )
                when :Page
                    pdf, idx = args
                    size = 22.0 - ( idx.length * 2.0 )
                    if size < 14.0
                        size = 14.0
                    end
                    pdf.newline
                    pdf.textPara( "NewCenturySchlbk-Roman", size, LEFT, 0.0, 0.4, 
                        "#{idx.join('.')}. #{self.title}", idx.length == 1 ) 
                    self.elements.collect { |e| 
                        e.to_pdf( pdf ) 
                    }
			end
		end

		def to_chm( page )
			case page
				when :TableOfContents
					return <<EOF
<LI> <OBJECT type="text/sitemap">
     <param name="Name" value="#{@title}">
	 <param name="Local" value="#{self.html_file_name}">
     </OBJECT>
EOF
				when :Html
					return self.to_html( :Html )
			end
		end

		def to_html( page )
			case page
				when :TableOfContents
					return <<EOF
<LI> <A HREF="#{self.html_file_name}" TARGET="docview">#{@title}</A>
EOF
				when :Html
					return <<EOF
<html>
<head>
	<link href="../global.css" type="text/css" rel="stylesheet">
	<title>#{@title}</title>
</head>

<body>
<div class="sectionhead">#{@title}</div>
#{self.elements.collect { |e| e.to_html( :Html ) }}
</body>
</html>
EOF
			end
		end
	end

	class CodePage < Page
		def initialize( data )
			super( data )
			ctr = 0
			@elements.each { |e|
				e.text['no'] = ( ctr += 1 ) if Yod::Code === e
			}
		end
	end

	class ClassDef < Group
		attr_accessor :const_name
		def title=( t )
			@title = t
			@const_name = t
			@elements.each { |e|
				e.add_const_namespace( self.const_namespace )
			}
		end
		def add_const_namespace( ns )
			self.const_name = ns + self.const_name
			self.elements.each { |e|
				e.add_const_namespace( ns )
			}
		end
		def const_namespace
			"#{@title}#"
		end
		def title
			"#{@const_name} Class"
		end
	end

	class ModuleDef < ClassDef
		def const_namespace
			"#{@title}::"
		end
		def title
			"#{@const_name} Module"
		end
	end

	class Method < DocElement
		attr_accessor :brief, :since, :arguments, :block, :details, :returns, :const_name
		def initialize( data )
			if Hash === data
				[:brief, :since, :arguments, :block, :returns, :details].each { |a|
					self.send( "#{a.id2name}=", data[a.id2name] )
				}
			else
				raise Yod::Error, "ClassMethod must be a Hash."
			end
		end

		def add_const_namespace( ns )
			self.const_name = ns + self.const_name
		end

		def html_file_name
			"class" + File::SEPARATOR + self.title.downcase.gsub( /\W+/, '_' ) + ".htm"
		end

		def title=( t )
			@title = t
			@const_name = t
		end

		def title
			"#{@const_name} Method"
		end

		def index
			index_all = { self.title => self.html_file_name }
		end

		def method_example
			case self.const_name
				when /^(.+)#(\w+)$/
					meth = $2
					classObj = $1.gsub( /[A-Z]+/ ) { |s| s[0..0] + s[1..-1].downcase }.gsub( /::/, '' )
					"a#{classObj}.#{meth}"
				else
					self.const_name
			end
		end

        def to_man( *args )
            page = args.shift
			case page
                when :Page
                    depth = args.shift
                    if depth.zero?
                        str = ".SH #{self.title}\n"
                    else
                        str = ".Sh #{self.title}\n"
                    end
                    str
            end
        end

		def to_pdf( *args )
            page = args.shift
			case page
				when :TableOfContents
                    pdf, idx = args
                    pdf.textCRLFshow( "#{idx}. #{self.title}" )
                when :Page
                    pdf, idx = args
                    txt = {}
					if Array === @arguments
						txt['args'] = "\n" + @arguments.collect { |p| 
                            if Array === p['type'] 
                                "  (" + p['type'].join( " or " ) + ") #{p['name']}" 
                            else
                                "  (#{p['type']}) #{p['name']}" 
                            end
						}.join( ",\n" ) + "\n"
					end

                    pdf.newline
                    pdf.textPara( "NewCenturySchlbk-Roman", 16.0, LEFT, 0.0, 
                        0.4, "#{idx.join('.')}. #{self.title}" ) 
                    pdf.textPara( "NewCenturySchlbk-Roman", 10.0, LEFT, 0.0, 0.2, @brief ) 
                    pdf.textPara( "Courier-Bold", 12.0, LEFT, 0.3, 
                        0.2, "#{self.method_example}(#{txt['args']})" )
                    pdf.newline
                    pdf.textPara( "NewCenturySchlbk-Roman", 14.0, LEFT, 0.0, 
                        0.2, "Parameters" )
					if Array === @arguments
                        @arguments.each { |p|
                            pdf.textPara( "NewCenturySchlbk-Italic", 10.0, LEFT, 0.2, 
                                0.1, p['name'] ) 
                            pdf.textPara( "NewCenturySchlbk-Roman", 10.0, LEFT, 0.4, 
		                        0.2, p['brief'] )
						}
                    else
                        pdf.textPara( "NewCenturySchlbk-Roman", 10.0, LEFT, 0.2, 
                            0.2, "None" )
                    end
					if Array === @block
                        pdf.newline
                        pdf.textPara( "NewCenturySchlbk-Roman", 14.0, LEFT, 0.0, 
                            0.2, "Block Parameters" )
                        @block.collect { |p|
                            pdf.textPara( "NewCenturySchlbk-Italic", 10.0, LEFT, 0.2, 
                                0.1, p['name'] ) 
                            pdf.textPara( "NewCenturySchlbk-Roman", 10.0, LEFT, 0.4, 
		                        0.2, p['brief'] )
						} 
                    end
                    pdf.newline
                    pdf.textPara( "NewCenturySchlbk-Roman", 14.0, LEFT, 0.0, 
                        0.2, "Returns" )
                    pdf.textPara( "NewCenturySchlbk-Roman", 10.0, LEFT, 0.4, 
                        0.2, @returns || "None" )
                    if Array === @details
                        pdf.newline
                        pdf.textPara( "NewCenturySchlbk-Roman", 14.0, LEFT, 0.0, 
                            0.2, "Details" )
                        @details.each { |e|  
                            e.to_pdf( pdf ) 
                        }
                    end
			end
		end

		def to_chm( page )
			case page
				when :TableOfContents
					return <<EOF
<LI> <OBJECT type="text/sitemap">
     <param name="Name" value="#{self.title}">
	 <param name="Local" value="#{self.html_file_name}">
     </OBJECT>
EOF
				when :Html
                    return self.to_html( :Html )
            end
        end

		def to_html( page )
			case page
				when :TableOfContents
					return <<EOF
<LI> <A HREF="#{self.html_file_name}" TARGET="docview">#{self.title}</A>
EOF
				when :Html
					ht = {}
					if Array === @details
						ht['remarks'] = <<EOF
	<!-- Remarks -->
	<div class="indent1"><strong>Details</strong></div>
	<div class="indent2">
	#{@details.collect { |e| e.to_html( :Html ) }}
	</div>
	<BR>
EOF
					end

					if Array === @arguments
						ht['args'] = "\n" + @arguments.collect { |p| 
							if Array === p['type']
								"  <strong>(" + p['type'].join( " or " ) + ")</strong> #{p['name']}" 
							else
								"  <strong>#{p['type']}</strong> #{p['name']}" 
							end
						}.join( ",\n" ) + "\n"
						
						ht['args2'] = @arguments.collect { |p| <<EOF
		<DT>#{p['name']}</DT>
		<DD>#{Yod.escapeHTML( p['brief'] )}</DD>
EOF
						}.join( "\n" )
					else
						ht['args2'] = "<DT>None</DT>"
					end

					if Array === @block
						ht['blockvars'] = @block.collect { |p| <<EOF
		<DT>#{p['name']}</DT>
		<DD>#{Yod.escapeHTML( p['brief'] )}</DD>
EOF
						}.join( "\n" )
						
						ht['blockvars'] = <<EOF
	<!-- Block Vars -->
	<div class="indent1"><strong>Block Parameters</strong></div>
	<DL>
	#{ht['blockvars']}
	</DL>
EOF
					end
	
					return <<EOF
<html>
<head>
	<link href="../global.css" type="text/css" rel="stylesheet">
	<title>#{self.title}</title>
</head>

<body>
	<div class="functionhead"><img src="../section.gif" alt="" width="15" height="15" border="0" align="middle">&nbsp;&nbsp;#{self.title}</div>
	
	<!-- Function Name -->
	<div class="function">#{@title}</div>
	
	<!-- Summary -->
	<div class="indent2">#{Yod.escapeHTML( @brief )}
	</div>
	<BR>
	
	<!-- Function Declaration -->
	<div class="indent2">
<pre class="codeblock"><strong>#{self.method_example}(</strong>#{ht['args']})</pre>
	</div>
	<BR>
	
	<!-- Parameters -->
	<div class="indent1"><strong>Parameters</strong></div>
	<DL>
	#{ht['args2']}
	</DL>
	
	#{ht['blockvars']}

	<!-- Return values -->
	<div class="indent1"><strong>Return Values</strong></div>
	<div class="indent2">#{@returns || "None"}</div>
	<BR>
	
	#{ht['remarks']}
	
	<!-- See Also
	<div class="indent1"><strong>See Also</strong></div>
	<div class="indent2">
	See_Also
	</div>
	-->
	
</body>
</html>	
EOF
			end
		end
	end

	class PageElement 
		attr_accessor :text
		def initialize( text )
			@text = text
		end
	end

	class Paragraph < PageElement
        def to_man
            "#{text}\n.LP\n"
        end
        def to_pdf( pdf )
            pdf.textPara( "NewCenturySchlbk-Roman", 10.0, LEFT, 0.0, 0.2, "#{text}" ) 
        end
		def to_html( page )
			case page
				when :Html
					return <<EOF
<P>
#{Yod.escapeHTML( text )}
</P>
EOF
			end
		end
	end

	class Code < PageElement
        def to_man
            "#{text['code']}\n.LP\n"
        end
        def to_pdf( pdf )
            pdf.textPara( "CPDF-Monospace", 12.0, LEFT, 0.3, 0.2, text['code'] ) 
            pdf.textPara( "NewCenturySchlbk-Italic", 10.0, CENTER, 0.3, 
                0.2, "Ex. #{text['no']}: #{Yod.escapeHTML( text['name'] )}" ) 
        end
		def to_html( page )
			case page
				when :Html
					return <<EOF
<pre class="codeblock">#{Yod.escapeHTML( text['code'] )}</pre>
<center><i>Ex. #{text['no']}: #{Yod.escapeHTML( text['name'] )}</i></center>
EOF
			end
		end
	end

	class Quote < PageElement
        def to_man
            ".IP\n#{text}\n"
        end
        def to_pdf( pdf )
            pdf.textPara( "NewCenturySchlbk-Italic", 10.0, LEFT, 0.3, 0.2, "#{text}" ) 
        end
		def to_html( page )
			case page
				when :Html
					return <<EOF
<BLOCKQUOTE>
<I>#{Yod.escapeHTML( text )}</I>
</BLOCKQUOTE>
EOF
			end
		end
	end

	class Title < PageElement
        def to_man
            ".Sh #{text}\n"
        end
        def to_pdf( pdf )
            pdf.newline
            pdf.textPara( "NewCenturySchlbk-Roman", 14.0, LEFT, 0.0, 0.2, "#{text}" ) 
        end
		def to_html( page )
			case page
				when :Html
					return <<EOF
<div class="quickhead">#{Yod.escapeHTML( text )}</div>
EOF
			end
		end
	end

	YAML.add_domain_type( "yaml4r.sf.net,2003", /^yod\// ) { |type, val|
		type =~ /^yod\/(\w+)(?::?(\w+))?/
		if Yod.const_defined?( $1 )
			if $2
				Yod.const_get( $1 ).new( $2, val )
			else
				Yod.const_get( $1 ).new( val )
			end
		else
			raise Yod::Error, "Invalid type #{type} not available in Yod module."
		end
	}

	def Yod.load( io )
		YAML::load( io )
	end

	# based on code by WATANABE Tetsuya
	def Yod.multi_mkdir( mpath, mask )
		path = ''
		mpath.split( File::SEPARATOR ).each do |f|
			path.concat( f )
			Dir.mkdir( path, mask ) unless path == '' || File.exist?( path )
			path.concat( File::SEPARATOR )
		end
	end

	def Yod.escapeHTML( string )
		string.to_s.gsub(/&/n, '&amp;').gsub(/\"/n, '&quot;').gsub(/>/n, '&gt;').gsub(/</n, '&lt;')
	end
end

