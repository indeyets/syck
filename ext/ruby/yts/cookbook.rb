#												vim:sw=4:ts=4
# $Id$
#
# Cookbook Generator for YamlTestingSuite
#
require 'erb/erbl'
require 'yaml'

ytsFiles = [
	[ 'YtsBasicTests.yml', {
		'name' => 'Collections',
		'doc' => []
	} ],
	[ 'YtsFlowCollections.yml', {
		'name' => 'Inline Collections',
		'doc' => []
	} ],
	[ 'YtsTypeTransfers.yml', {
		'name' => 'Basic Types',
		'doc' => []
	} ],
	[ 'YtsFoldedScalars.yml', {
		'name' => 'Blocks',
		'doc' => []
	} ],
	[ 'YtsAnchorAlias.yml', {
		'name' => 'Aliases and Anchors',
		'doc' => []
	} ],
	[ 'YtsDocumentSeparator.yml', {
		'name' => 'Documents',
		'doc' => []
	} ],
	[ 'YtsRubyTests.yml', {
		'name' => 'YAML For Ruby',
		'doc' => []
	} ]
# 	[ 'YtsSpecificationExamples.yml', {
# 		'name' => 'Examples from the Specification',
# 		'doc' => []
# 	} ]
]

ytsFiles.each do |yt|
	yt[1]['doc'] = YAML::load_stream( File.open( yt[0] ) )
    yt[1]['href'] = yt[1]['name'].downcase.gsub( /\s+/, '_' )
end

erb = ERbLight.new( <<TMPL

<a NAME="__index__"></a>

<!-- INDEX BEGIN -->
<ul>
<% ytsFiles.each do |yt| %>

	<li><a HREF="#<%= yt[1]['href'] %>"><%= yt[1]['name'] %></a></li>
	<%		if yt[1]['doc'].documents.length > 0 %>
	<ul>
		<% 
            yt[1]['doc'].documents.each do |ydoc| 
                ydoc['href'] = ydoc['test'].downcase.gsub( /\s+/, '_' ) if ydoc['test']
        %>
		<li><a HREF="#<%= ydoc['href'] %>"><%= ydoc['test'] %></a></li>
		<% 
            end 
        %>
	</ul>
	<% end %>

<% end %>
</ul>
<!-- INDEX END -->

<% ytsFiles.each do |yt| %>
<!-- <%= yt[1]['name'].upcase %> START -->

<p>
<hr>
<h1><a NAME="<%= yt[1]['href'] %>"><%= yt[1]['name'] %></a></h1>
</p>

<%	if yt[1]['doc'].documents.length > 0 
		yt[1]['doc'].documents.each do |ydoc| 		
%>

<h2><a NAME="<%= ydoc['href'] %>"><%= ydoc['test'] %></a></h2>

<div style="padding: 0px 0px 20px 20px; margin: 0px 0px 0px 0px;">

<p>
<h3>Brief</h3>
</p>

<p><%= ydoc['brief'] %></p>

<p>
<h3>Yaml</h3>
</p>

<table CELLPADDING="1" CELLSPACING="0" BORDER="0" CLASS="yaml-box-border" WIDTH="500">
<tr><td>
	<table WIDTH="500" BORDER="0" CELLPADDING="0" CELLSPACING="0" CLASS="yaml-box-body">
	<tr><td>
		<table BORDER="0" CELLPADDING="0" CELLSPACING="0">
			<tr><td CLASS="yaml-box-header">&nbsp;<%= ydoc['test'].gsub( ' ', '&nbsp;' ) if ydoc['test'] %>&nbsp;</td>
			<td><tt>&nbsp;</tt></td>
			<td CLASS="yaml-box-link">&nbsp;in&nbsp;YAML?&nbsp;</td></tr>
		</table>
	</td></tr>
	<tr><td>
		<table CELLPADDING="12" CELLSPACING="0">
		<tr><td><pre><%= ydoc['yaml'] %></pre>
		</td></tr>
		</table>
	</td></tr>
	</table>
</td></tr>
</table>

<p>
<h3>Ruby</h3>
</p>

<table CELLPADDING="1" CELLSPACING="0" BORDER="0" CLASS="ruby-box-border" WIDTH="500">
<tr><td>
	<table WIDTH="500" BORDER="0" CELLPADDING="0" CELLSPACING="0" CLASS="ruby-box-body">
	<tr><td>
		<table BORDER="0" CELLPADDING="0" CELLSPACING="0">
			<tr><td CLASS="ruby-box-header">&nbsp;<%= ydoc['test'].gsub( ' ', '&nbsp;' ) if ydoc['test'] %>&nbsp;</td>
			<td><tt>&nbsp;</tt></td>
			<td CLASS="ruby-box-link">&nbsp;in&nbsp;Ruby?&nbsp;</td></tr>
		</table>
	</td></tr>
	<tr><td>
		<table CELLPADDING="12" CELLSPACING="0">
		<tr><td>
		<% if ydoc.has_key?( 'ruby-setup' ) %>
		<div CLASS="ruby-box-highlight"><pre><%= ydoc['ruby-setup'] %></pre></div>
		<% end %>
		<pre><%= ydoc['ruby'] %></pre>
		</td></tr>
		</table>
	</td></tr>
	</table>
</td></tr>
</table>

</div>

<% 		end 
	end 		%>

<!-- <%= yt[1]['name'].upcase %> END -->
<% end %>

TMPL
	)
puts erb.result
