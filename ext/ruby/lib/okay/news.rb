#
# The !okay/news module for YAML.rb
# 
require 'okay'

module Okay
 
  class News < ModuleBase
    attr_accessor :title, :link, :description, :updatePeriod, :items
    def to_yaml_properties
      [ '@title', '@link', '@description', '@updatePeriod', '@items' ]
    end
    def to_yaml_type
      "!okay/news"
    end
  end

  Okay.add_type( "news" ) { |type, val, modules|
    Okay.object_maker( Okay::News, val, modules )
  }

  class NewsItem < ModuleBase
    attr_accessor :title, :link, :description, :pubTime
    def to_yaml_properties
      [ '@title', '@link', '@description', '@pubTime' ]
    end
    def to_yaml_type
      "!okay/news/item"
    end
  end

  Okay.add_type( "news/item" ) { |type, val, modules|
    Okay.object_maker( Okay::NewsItem, val, modules )
  }

end

Okay.load_schema( <<EOY )

# Schema for Okay::News types
--- %YAML:1.0 !okay/schema
okay/news: 
  description: >
    Inspired by RSS, more limited...
  examples: >
    If I had a news site... 
  schema:  
    - map: 
        /title: [ str ]
        /link: [ str ]
        /description: [ str ]  
        /updatePeriod: [ str ] 
        /items: 
          - seq: { /*: [ okay/news/item ] }

okay/news/item: 
  description: >
    Inside okay/news lies... 
  examples: > 
    See okay/news examples...
  schema:  
    - map: 
        /title: [ str ] 
        /pubTime: [ time ]
        /link: [ str ]
        /description: [ str ]
        optional: [ /title ]

EOY
