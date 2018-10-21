#!/usr/bin/perl -w

# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

#
# Run `pod2text` on this file to generate the man page.
#

=pod

=encoding utf8

=head1 NAME

run_benchmark.pl - Generates PDF output from picotm-perf benchmarks

=head1 SYNOPSIS

run_benchmark.pl

=head1 DESCRIPTION

The tool run_benchmark.pl runs picotm-perf with various combinations of
parameters and generates a PDF documents from the results. The resulting
document C<results.pdf> can be found under
C<benchmark-E<lt>current dateE<gt>/>. The current date is the number of
seconds since I<1970-01-01 00:00:00 UTC.>

run_benchmark.pl is implemented in Perl 5.22. It requires LaTeX and
gnuplot for generating the output.

=head1 AUTHOR

Thomas Zimmermann L<contact@tzimmermann.org>

=head1 REPORTING BUGS

Please report bugs to L<picotm-devel@freelists.org>.

=head1 COPYRIGHT

Copyright (c) 2017 Thomas Zimmermann. This program is distributed under
the terms of the MIT license.

=head1 SEE ALSO

picotm is available at: L<http://picotm.org>

=cut

use v5.22;
use autodie;
use strict;
use warnings;

use File::Path qw(make_path remove_tree);
use File::Temp qw/ tempfile tempdir /;

my $PDF_FILENAME = 'results.pdf';

my $TIME_MSECS = 60000;
my $OUTDIR = 'benchmark-' . `date -u +%s`;
chomp $OUTDIR;

# picotm-perf
my $PICOTM_PERF = 'picotm-perf';
my $PICOTM_PERF_FLAGS = '';

# LaTeX
my $LATEX = 'pdflatex';
my $LATEX_FLAGS = '-file-line-error -halt-on-error -shell-escape';

#
# System information
#

sub get_nprocessors_info {

    my $nprocessors_info = `cat /proc/cpuinfo | grep processor | sort | wc -l`;
    chomp $nprocessors_info;

    return $nprocessors_info;
}

sub get_processor_info {

    my $processor_info = (`cat /proc/cpuinfo | grep model\\ name | uniq | awk -F ':' '{print \$2}'` or
                          'Unknown processor');
    chomp $processor_info;

    return $processor_info;
}

sub get_memory_info {

    my $memory_info = (`free | grep Mem\\: | awk '{print \$2 \" KiB total / " \$3 " KiB used / " \$4 " KiB free"}'` or
                       'Unknown amount of memory');
    chomp $memory_info;

    return $memory_info;
}

sub get_distribution_info {

    my $distribution_info = (`cat /etc/fedora-release` or
                             `cat /etc/redhat-release` or
                             'Unknown Linux distribution');
    chomp $distribution_info;

    return $distribution_info;
}

sub get_kernel_info {

    my $kernel_info = `uname -srv` or die "uname failed: $?";
    chomp $kernel_info;

    return $kernel_info;
}

#
# Data files
#

# Generates a canonical name for a data file
#
sub generate_dat_filename {

    my ($nthreads, $nmsecs, $pattern, $nloads, $nstores) = @_;

    return "perf-$pattern-$nmsecs-$nthreads-$nloads-$nstores.dat"
}

# Saves results to a data file
#
sub generate_dat_file {

    my ($nthreads, $nmsecs, $pattern, $nloads, $nstores, $dat_string) = @_;

    my ($fh, $filename) = tempfile(DIR => '.');

    print $fh $dat_string;
    close $fh;

    my $dat_filename = generate_dat_filename($nthreads, $nmsecs, $pattern, $nloads, $nstores);
    rename $filename, $dat_filename;

    return $dat_filename;
}

# Run `picotm-perf` utility with the given parameters
#
sub run_picotm_perf {

    my ($nthreads, $nmsecs, $pattern, $nloads, $nstores) = @_;

    my $result_string = `$PICOTM_PERF $PICOTM_PERF_FLAGS -t $nthreads -T $nmsecs -P $pattern -L $nloads -S $nstores` or
        die "$PICOTM_PERF failed: $1";

    return $result_string;
}

sub filter_result_string {

    my ($nthreads, $result_string) = @_;

    my $ncommits = 0;
    my $nretries = 0;

    my @lines = split /\n/, $result_string;

    foreach my $line (@lines) {

        # <thread id> <nmsecs> <ncommits> <nretries>
        $line =~ m/^(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s*$/ or die;

        # Sum up normalized results
        $ncommits += $3 * 1000 / $2;
        $nretries += $4 * 1000 / $2;
    }

    return "$nthreads $ncommits $nretries";
}

sub generate_dat {

    my ($nprocessors, $nmsecs, $pattern, $nloads, $nstores) = @_;

    my $dat_string = '# <nthreads> <ncommits> <nretries>';

    foreach my $nthreads (1 .. $nprocessors) {

        my $result_string = run_picotm_perf($nthreads, $nmsecs, $pattern,
                                            $nloads, $nstores);

        $dat_string .= "\n" . filter_result_string($nthreads, $result_string);
    }

    return $dat_string;
}

#
# gnuplot
#

sub generate_gp {

    my ($nprocessors, $dat_filename) = @_;

    my $max_xrange = $nprocessors + 1;

    my $gp_string = '';
    $gp_string .= "\n" . 'set xlabel "Number of concurrent transactions"';
    $gp_string .= "\n" . "set xrange [0:$max_xrange]";
    $gp_string .= "\n" . 'set xtics 1';
    $gp_string .= "\n" . 'set ylabel "Commits per second" rotate by 90';
    $gp_string .= "\n" . 'set yrange [0:]';
    $gp_string .= "\n" . 'set ytics';
    $gp_string .= "\n" . 'set y2label "Retries per second" rotate by 270';
    $gp_string .= "\n" . 'set y2range [0:]';
    $gp_string .= "\n" . 'set y2tics nomirror';
    $gp_string .= "\n" . 'set style fill solid';
    $gp_string .= "\n" . 'set boxwidth 0.25 relative';
    $gp_string .= "\n" . "plot \"$dat_filename\" using (\$1-0.125):2 lc rgb \"blue\" with boxes axes x1y1 title \"(Commits/s)\",";
    $gp_string .=            " \"$dat_filename\" using (\$1+0.125):3 lc rgb \"red\"  with boxes axes x1y2 title \"(Retries/s)\"";

    return $gp_string;
}

# Generates a temporary gnuplot file
#
sub generate_gp_file {

    my ($nprocessors, $dat_filename, $tex_filename) = @_;

    my ($fh, $filename) = tempfile(DIR => '.');

    say $fh 'set terminal latex';
    say $fh "set output";
    say $fh generate_gp($nprocessors, $dat_filename);
    close $fh;

    return $filename;
}

#
# LaTeX files
#

sub escape_latex_string {

    my ($string) = @_;

    $string =~ s/\\/\\textbackslash/g;
    $string =~ s/\^/\\textasciicircum/g;
    $string =~ s/\\/\\textasciitilde/g;
    $string =~ s/\&/\\\&/g;
    $string =~ s/\%/\\\%/g;
    $string =~ s/\$/\\\$/g;
    $string =~ s/\#/\\\#/g;
    $string =~ s/\_/\\\_/g;
    $string =~ s/\{/\\\{/g;
    $string =~ s/\}/\\\}/g;

    return $string;
}

sub write_latex_header {

    my ($tex_fh, $title, $author) = @_;

    say $tex_fh '\documentclass[10pt, a4paper]{article}%';
    say $tex_fh '\usepackage{float}%';
    say $tex_fh '\usepackage{gnuplottex}%';
    say $tex_fh '\usepackage{graphicx}%';
    say $tex_fh '\usepackage{hyperref}%';
    say $tex_fh '\usepackage{ifthen}%';
    say $tex_fh '\usepackage{latexsym}%';
    say $tex_fh '\usepackage{moreverb}%';
    say $tex_fh '\hypersetup{%';
    say $tex_fh "    pdftitle    = {$title},";
    say $tex_fh "    pdfauthor   = {$author},";
    say $tex_fh '    pdfkeywords = {benchmark, performance, picotm}';
    say $tex_fh '}';
    say $tex_fh "\\title{$title}%";
    say $tex_fh "\\author{$author}%";
    say $tex_fh '\date{\today}%';
    say $tex_fh '\begin{document}%';
    say $tex_fh '\maketitle%';
    say $tex_fh '\vfill%';
    say $tex_fh '\tableofcontents%';
    say $tex_fh '\vfill%';
}

sub write_latex_frontpage {

    my ($tex_fh) = @_;

    my $processor_info = escape_latex_string(get_processor_info());
    my $nprocessors_info = escape_latex_string(get_nprocessors_info());
    my $memory_info = escape_latex_string(get_memory_info());
    my $distribution_info = escape_latex_string(get_distribution_info());
    my $kernel_info = escape_latex_string(get_kernel_info());

    say $tex_fh '\newpage%';
    say $tex_fh '\section{System Information}%';
    say $tex_fh '\begin{table}[hp]%';
    say $tex_fh "\\label{tab:system_information}%";
    say $tex_fh '\begin{center}%';
    say $tex_fh '\begin{tabular}{ll}%';
    say $tex_fh "Processor & $processor_info\\\\%";
    say $tex_fh "Number of processors & $nprocessors_info\\\\%";
    say $tex_fh "Memory & $memory_info\\\\%";
    say $tex_fh '&\\\\%';
    say $tex_fh "Distribution & $distribution_info\\\\%";
    say $tex_fh "Kernel       & $kernel_info\\\\%";
    say $tex_fh '\end{tabular}%';
    say $tex_fh '\end{center}%';
    say $tex_fh "\\caption{System information}%";
    say $tex_fh '\end{table}%';
}

sub write_latex_chapter {

    my ($tex_fh, $title) = @_;

    say $tex_fh "\\chapter{$title}%";
}

sub write_latex_section {

    my ($tex_fh, $title) = @_;

    say $tex_fh '\newpage%';
    say $tex_fh "\\section{$title}%";
}

sub write_latex_figure {

    my ($tex_fh, $figure, $caption, $label) = @_;

    say $tex_fh '\begin{figure}[hp]%';
    say $tex_fh "\\label{fig:$label}%";
    say $tex_fh '\centering%';
    say $tex_fh $figure;
    say $tex_fh "\\caption{$caption}%";
    say $tex_fh '\end{figure}%';
}

sub write_latex_gnuplot {

    my ($tex_fh, $nprocessors, $pattern, $nloads, $nstores, $dat_filename) = @_;

    my $figure = '';
    $figure .= "\n" . '\begin{gnuplot}[terminal=epslatex]';
    $figure .= "\n" . generate_gp($nprocessors, $dat_filename);
    $figure .= "\n" . '\end{gnuplot}';

    my $caption = "I/O pattern \\emph{$pattern}, $nloads loads, $nstores stores";
    my $label   = $dat_filename;

    write_latex_figure($tex_fh, $figure, $caption, $label);
}

sub write_latex_dat {

    my ($tex_fh, $nprocessors, $nmsecs, $pattern, $nloads, $nstores) = @_;

    # Generate data and save into external file
    my $dat_string = generate_dat($nprocessors, $nmsecs, $pattern, $nloads,
                                  $nstores);

    my $dat_filename = generate_dat_file($nprocessors, $nmsecs, $pattern,
                                         $nloads, $nstores, $dat_string);

    # Write results to LaTeX document
    write_latex_gnuplot($tex_fh, $nprocessors, $pattern, $nloads, $nstores, $dat_filename);
}

sub write_latex_footer {

    my ($tex_fh) = @_;

    say $tex_fh '\newpage%';
    say $tex_fh '\listoffigures%';
    say $tex_fh '\newpage%';
    say $tex_fh '\listoftables%';
    say $tex_fh '\end{document}%';
}

# Generates a LaTeX picture from a data file
#
sub generate_latex_picture {

    my ($nprocessors, $dat_filename) = @_;

    my $gp_filename = generate_gp_file($nprocessors, $dat_filename);

    my $latex_picture = `cat $gp_filename | gnuplot`;

    unlink $gp_filename;

    return $latex_picture;
}

sub generate_latex_tempfile {

    my ($nprocessors, $nmsecs) = @_;

    my ($fh, $filename) = tempfile(DIR => '.');

    write_latex_header($fh, 'picotm Performance Tests', 'picotm-perf Utility');
    write_latex_frontpage($fh);

    foreach my $pattern ('random', 'sequential') {

        write_latex_section($fh, "I/O Pattern \\emph{$pattern}");

        foreach my $nloads (0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100) {

            my $nstores = 100 - $nloads;
            write_latex_dat($fh, $nprocessors, $nmsecs, $pattern, $nloads, $nstores);
        }
    }

    write_latex_footer($fh);

    close $fh;

    return $filename;
}

sub generate_latex_file {

    my ($tex_filename, $nprocessors, $nmsecs) = @_;

    my $filename = generate_latex_tempfile($nprocessors, $nmsecs);
    rename $filename, $tex_filename;
}

#
# PDF output
#

sub generate_pdf {

    my ($pdf_filename, $nprocessors, $nmsecs) = @_;

    my $tex_filename = $pdf_filename;
    $tex_filename =~ s/\.pdf$/\.tex/g;
    generate_latex_file($tex_filename, $nprocessors, $nmsecs);

    system("$LATEX $LATEX_FLAGS -draft $tex_filename > /dev/null") == 0 or die "$LATEX failed: $?";
    system("$LATEX $LATEX_FLAGS -draft $tex_filename > /dev/null") == 0 or die "$LATEX failed: $?";
    system("$LATEX $LATEX_FLAGS $tex_filename > /dev/null") == 0 or die "$LATEX failed: $?";
}

#
# Main logic
#

my $nprocessors = eval get_nprocessors_info();

make_path($OUTDIR);
chdir $OUTDIR;
generate_pdf($PDF_FILENAME, $nprocessors, $TIME_MSECS);

say "Test results have been written to $OUTDIR/$PDF_FILENAME.";
